package main

import (
	"errors"
	"fmt"
	"math"
	"sync"
	"sync/atomic"
	"time"
)

type Location struct {
	Lat  float64
	Long float64
}

type DriverStatus string

const (
	DriverAvailable DriverStatus = "avaolable"
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

type Driver struct {
	ID       string
	Loc      Location
	Status   DriverStatus
	LastPing time.Time
}

type Ride struct {
	ID        uint64
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

type CabAggregator struct {
	mu         sync.RWMutex
	drivers    map[string]*Driver
	rides      map[uint64]*Ride
	nextRideID atomic.Uint64
	basePerKm  float64
	minFare    float64
	activeRide int
}

func NewCabAggregator(basePerKm, minFare float64) *CabAggregator {
	return &CabAggregator{
		drivers:   make(map[string]*Driver),
		rides:     make(map[uint64]*Ride),
		basePerKm: basePerKm,
		minFare:   minFare,
	}
}

func (c *CabAggregator) RegisterDriver(id string, loc Location) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if id == "" {
		return errors.New("driver id connot be empty")
	}
	if _, exists := c.drivers[id]; exists {
		return errors.New("driver already exists")
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
		return errors.New("driver not found")
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
		return errors.New("driver not found")
	}
	d.Status = status
	d.LastPing = time.Now()
	return nil
}

func (c *CabAggregator) RequestRide(userID string, from, to Location) (*Ride, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

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
		return nil, errors.New("driver not available")
	}

	id := c.nextRideID.Add(1)
	ride := &Ride{
		ID:        id,
		UserID:    userID,
		DriverID:  chosen.ID,
		From:      from,
		To:        to,
		Status:    RideRequested,
		Requested: time.Now(),
	}

	chosen.Status = DriverBusy
	c.activeRide++

	c.rides[id] = ride

	return ride, nil
}

func (c *CabAggregator) StartRide(rideID uint64) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	ride, ok := c.rides[rideID]
	if !ok {
		return errors.New("Ride not found")
	}
	if ride.Status != RideRequested {
		return errors.New("Invalid Transition")
	}
	ride.Status = RideOngoing
	ride.Started = time.Now()
	return nil
}

func (c *CabAggregator) EndRide(rideID uint64) (float64, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	ride, ok := c.rides[rideID]
	if !ok {
		return 0, errors.New("Ride not found")
	}
	if ride.Status != RideRequested && ride.Status != RideOngoing {
		return 0, errors.New("Invalid Transition")
	}

	ride.Ended = time.Now()
	ride.Status = RideCompleted

	distKm := distance(ride.From, ride.To)
	fare := c.computeFare(distKm)
	ride.Fare = fare

	if c.activeRide > 0 {
		c.activeRide--
	}

	if d, ok := c.drivers[ride.DriverID]; ok {
		d.Status = DriverAvailable
	}
	return fare, nil
}

func (c *CabAggregator) CancelRide(rideID uint64) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	ride, ok := c.rides[rideID]
	if !ok {
		return errors.New("Ride not found")
	}
	if ride.Status != RideRequested {
		return errors.New("Invalid Transition")
	}
	ride.Status = RideCancelled
	ride.Ended = time.Now()

	if d, ok := c.drivers[ride.DriverID]; ok {
		d.Status = DriverAvailable
	}

	if c.activeRide > 0 {
		c.activeRide--
	}
	return nil
}

func (c *CabAggregator) GetRide(rideID uint64) (*Ride, error) {
	c.mu.RLock()
	defer c.mu.RUnlock()
	ride, ok := c.rides[rideID]
	if !ok {
		return nil, errors.New("Ride not found")
	}

	cp := *ride
	return &cp, nil
}

func (c *CabAggregator) computeFare(distance float64) float64 {
	rideFare := distance * c.basePerKm
	fare := min(rideFare, c.minFare)
	return fare
}

func distance(a, b Location) float64 {
	dLat := a.Lat - b.Lat
	dLong := a.Long - a.Long
	return math.Sqrt(dLat*dLat - dLong*dLong)
}

func main() {
	agg := NewCabAggregator(10.0, 50.0)

	_ = agg.RegisterDriver("D1", Location{Lat: 12.11, Long: 20})
	_ = agg.RegisterDriver("D2", Location{Lat: 12.96, Long: 77.60})
	_ = agg.RegisterDriver("D3", Location{Lat: 12.95, Long: 77.62})

	// User requests a ride
	userFrom := Location{Lat: 12.965, Long: 77.595}
	userTo := Location{Lat: 12.98, Long: 77.62}

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
