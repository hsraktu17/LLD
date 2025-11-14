package main

import (
	"errors"
	"fmt"
	"math"
	"sync"
	"sync/atomic"
	"time"
)

// ---- Domain Types ----

type DriverStatus string

const (
	DriverAvailable DriverStatus = "available"
	DriverBusy      DriverStatus = "busy"
	DriverOffline   DriverStatus = "offline"
)

type RideStatus string

const (
	RideRequested RideStatus = "requested"
	RideOngoing   RideStatus = "ongoing"
	RideCompleted RideStatus = "completed"
	RideCancelled RideStatus = "cancelled"
)

type Location struct {
	Lat float64
	Lon float64
}

type Driver struct {
	ID       string
	Loc      Location
	Status   DriverStatus
	LastPing time.Time
	// Optional: metrics like completed rides, total earnings, rating etc.
}

type RideID uint64

type Ride struct {
	ID        RideID
	UserID    string
	DriverID  string
	From      Location
	To        Location
	Status    RideStatus
	Requested time.Time
	Started   time.Time
	Ended     time.Time
	Fare      float64
}

// ---- Cab Aggregator Core ----

type CabAggregator struct {
	mu sync.RWMutex

	drivers map[string]*Driver
	rides   map[RideID]*Ride

	nextRideID atomic.Uint64

	// pricing config
	basePerKm float64 // e.g. 10 INR/km
	minFare   float64 // e.g. 50 INR

	// for surge pricing
	activeRides int // rides in Requested/Ongoing
}

// ---- Errors ----

var (
	ErrDriverExists       = errors.New("driver already exists")
	ErrDriverNotFound     = errors.New("driver not found")
	ErrRideNotFound       = errors.New("ride not found")
	ErrInvalidTransition  = errors.New("invalid ride state transition")
	ErrNoAvailableDrivers = errors.New("no available drivers")
)

// ---- Constructor ----

func NewCabAggregator(basePerKm, minFare float64) *CabAggregator {
	return &CabAggregator{
		drivers:   make(map[string]*Driver),
		rides:     make(map[RideID]*Ride),
		basePerKm: basePerKm,
		minFare:   minFare,
	}
}

// ---- Driver Operations ----

func (c *CabAggregator) RegisterDriver(id string, loc Location) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if id == "" {
		return errors.New("driver id cannot be empty")
	}
	if _, exists := c.drivers[id]; exists {
		return ErrDriverExists
	}

	c.drivers[id] = &Driver{
		ID:       id,
		Loc:      loc,
		Status:   DriverAvailable,
		LastPing: time.Now(),
	}
	return nil
}

func (c *CabAggregator) UpdateDriverLocation(id string, loc Location) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	d, ok := c.drivers[id]
	if !ok {
		return ErrDriverNotFound
	}
	d.Loc = loc
	d.LastPing = time.Now()
	return nil
}

func (c *CabAggregator) SetDriverStatus(id string, status DriverStatus) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	d, ok := c.drivers[id]
	if !ok {
		return ErrDriverNotFound
	}
	d.Status = status
	d.LastPing = time.Now()
	return nil
}

// ---- Ride Operations ----

// RequestRide finds nearest available driver, assigns ride, marks driver busy.
func (c *CabAggregator) RequestRide(userID string, from, to Location) (*Ride, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	// find nearest available driver
	var chosen *Driver
	var bestDist float64

	for _, d := range c.drivers {
		if d.Status != DriverAvailable {
			continue
		}
		dist := distance(from, d.Loc)
		if chosen == nil || dist < bestDist {
			chosen = d
			bestDist = dist
		}
	}

	if chosen == nil {
		return nil, ErrNoAvailableDrivers
	}

	// create ride
	id := RideID(c.nextRideID.Add(1))
	now := time.Now()

	ride := &Ride{
		ID:        id,
		UserID:    userID,
		DriverID:  chosen.ID,
		From:      from,
		To:        to,
		Status:    RideRequested,
		Requested: now,
	}

	// mark driver busy
	chosen.Status = DriverBusy

	// track active ride count
	c.activeRides++

	c.rides[id] = ride
	return ride, nil
}

// StartRide marks the ride as ongoing.
func (c *CabAggregator) StartRide(rideID RideID) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	ride, ok := c.rides[rideID]
	if !ok {
		return ErrRideNotFound
	}
	if ride.Status != RideRequested {
		return ErrInvalidTransition
	}
	ride.Status = RideOngoing
	ride.Started = time.Now()
	return nil
}

// EndRide marks the ride as completed, computes final fare, and frees driver.
func (c *CabAggregator) EndRide(rideID RideID) (float64, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	ride, ok := c.rides[rideID]
	if !ok {
		return 0, ErrRideNotFound
	}
	if ride.Status != RideOngoing && ride.Status != RideRequested {
		return 0, ErrInvalidTransition
	}

	now := time.Now()
	if ride.Started.IsZero() {
		// ride was never started; treat requested time as start
		ride.Started = ride.Requested
	}
	ride.Ended = now
	ride.Status = RideCompleted

	// compute distance and fare
	distKm := distance(ride.From, ride.To) // pretend this is km
	surge := c.computeSurgeFactorLocked()
	fare := c.computeFareLocked(distKm, surge)

	ride.Fare = fare

	// decrement active rides
	if c.activeRides > 0 {
		c.activeRides--
	}

	// set driver available again
	if d, ok := c.drivers[ride.DriverID]; ok {
		d.Status = DriverAvailable
	}

	return fare, nil
}

// CancelRide allows cancelling a ride before it starts.
// Simplified: if ride is Requested, we cancel and free the driver.
func (c *CabAggregator) CancelRide(rideID RideID) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	ride, ok := c.rides[rideID]
	if !ok {
		return ErrRideNotFound
	}
	if ride.Status != RideRequested {
		return ErrInvalidTransition
	}

	ride.Status = RideCancelled
	ride.Ended = time.Now()

	// free driver
	if d, ok := c.drivers[ride.DriverID]; ok {
		d.Status = DriverAvailable
	}

	// decrement active rides
	if c.activeRides > 0 {
		c.activeRides--
	}
	return nil
}

// GetRide returns a copy of a ride (read-only view).
func (c *CabAggregator) GetRide(rideID RideID) (*Ride, error) {
	c.mu.RLock()
	defer c.mu.RUnlock()

	ride, ok := c.rides[rideID]
	if !ok {
		return nil, ErrRideNotFound
	}
	// shallow copy to avoid external mutation
	cp := *ride
	return &cp, nil
}

// ---- Helper: Pricing & Surge ----

// computeSurgeFactorLocked must be called with c.mu held.
func (c *CabAggregator) computeSurgeFactorLocked() float64 {
	available := 0
	for _, d := range c.drivers {
		if d.Status == DriverAvailable {
			available++
		}
	}

	active := c.activeRides

	if available == 0 {
		// extreme surge when no one is free – but realistically,
		// you wouldn't accept new rides anyway.
		return 2.0
	}

	switch {
	case active <= available:
		return 1.0
	case active <= 2*available:
		return 1.5
	default:
		return 2.0
	}
}

func (c *CabAggregator) computeFareLocked(distKm, surge float64) float64 {
	raw := distKm * c.basePerKm * surge
	if raw < c.minFare {
		raw = c.minFare
	}
	return round2(raw)
}

// ---- Helper: Distance, Rounding ----

// Simple Euclidean distance; treat as "km" for this toy model.
func distance(a, b Location) float64 {
	dLat := a.Lat - b.Lat
	dLon := a.Lon - b.Lon
	return math.Sqrt(dLat*dLat + dLon*dLon)
}

func round2(x float64) float64 {
	if x < 0 {
		return 0
	}
	return math.Round(x*100) / 100
}

// ---- Small Demo in main() ----

func main() {
	agg := NewCabAggregator(10.0, 50.0) // 10 ₹/km, min 50 ₹

	// Register some drivers
	_ = agg.RegisterDriver("D1", Location{Lat: 12.97, Lon: 77.59})
	_ = agg.RegisterDriver("D2", Location{Lat: 12.96, Lon: 77.60})
	_ = agg.RegisterDriver("D3", Location{Lat: 12.95, Lon: 77.62})

	// User requests a ride
	userFrom := Location{Lat: 12.965, Lon: 77.595}
	userTo := Location{Lat: 12.98, Lon: 77.62}

	ride, err := agg.RequestRide("U1", userFrom, userTo)
	if err != nil {
		fmt.Println("RequestRide error:", err)
		return
	}
	fmt.Printf("Ride created: %+v\n", ride)

	// Start ride
	if err := agg.StartRide(ride.ID); err != nil {
		fmt.Println("StartRide error:", err)
		return
	}
	fmt.Println("Ride started")

	// Simulate travel time
	time.Sleep(1 * time.Second)

	// End ride
	fare, err := agg.EndRide(ride.ID)
	if err != nil {
		fmt.Println("EndRide error:", err)
		return
	}
	fmt.Printf("Ride ended, fare: %.2f\n", fare)

	// Inspect final ride
	finalRide, _ := agg.GetRide(ride.ID)
	fmt.Printf("Final ride state: %+v\n", finalRide)
}
