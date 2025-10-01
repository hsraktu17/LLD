package main

import (
	"fmt"
	"sort"
)

/*
Design and implement a Scalable Task Allocation System for customer success agents.


Each agent has a configurable maximum task capacity (default = 2).


A task should be assigned to an available agent based on:


Allotment Condition: Only assign if agent’s active tasks < capacity.


Tie-breaking Condition:


Fewer active tasks first. // Explain this


If tied, pick the agent who has been idle the longest.

I have to check the priority (this can be maintained by heap or priorityQueue?)

no need of manager Instead a task aloocator

*/

type Agent struct {
	ID           string
	Capacity     int
	ActiveTask   int
	LastAssigned int64
}

type TaskAllocator struct {
	Agents []Agent
	clock  int64
}

func (ta *TaskAllocator) AddAgent(Id string, capacity int) {

	if capacity <= 0 {
		capacity = 2
	}
	ta.Agents = append(ta.Agents, Agent{
		ID:       Id,
		Capacity: capacity,
	})
}

func (ta *TaskAllocator) AssignOneTask() (string, bool) {
	oneTask := -1
	for i := range ta.Agents {
		a := ta.Agents[i]
		if a.ActiveTask == a.Capacity {
			continue
		}
		if oneTask == -1 {
			oneTask = i
			continue
		}

		b := ta.Agents[oneTask]
		if a.ActiveTask < b.ActiveTask {
			oneTask = i
			continue
		}

		if a.ActiveTask > b.ActiveTask {
			continue
		}

		//tie breaker
		if a.LastAssigned < b.LastAssigned {
			oneTask = i
			continue
		}
		if a.LastAssigned > a.LastAssigned {
			continue
		}
	}

	if oneTask == -1 {
		return "", false
	}

	ta.Agents[oneTask].ActiveTask++
	ta.clock++
	ta.Agents[oneTask].LastAssigned = ta.clock

	return ta.Agents[oneTask].ID, true
}

func (ta *TaskAllocator) CompleteTask(Id string) bool {
	for i := range ta.Agents {
		if ta.Agents[i].ID == Id {
			if ta.Agents[i].ActiveTask > 0 {
				ta.Agents[i].ActiveTask--
				return true
			}
			return false
		}
	}
	return false
}

func (ta *TaskAllocator) Printer() []Agent {
	print := make([]Agent, len(ta.Agents))
	copy(print, ta.Agents)
	sort.Slice(print, func(i, j int) bool {
		return print[i].ID < print[i].ID
	})
	return print
}

func main() {
	var allocate TaskAllocator

	allocate.AddAgent("Utkarsh", 2)
	allocate.AddAgent("aditya", 2)
	allocate.AddAgent("Harshit", 2)

	fmt.Println("init", allocate.Printer())

	fmt.Println("---------One task-----------")
	if id, ok := allocate.AssignOneTask(); !ok {
		fmt.Println("No agent available")
	} else {
		fmt.Println("Assigned one task", id)
	}
	fmt.Println("one task assigned", allocate.Printer())

	fmt.Println("------------second task-----------")
	if id, ok := allocate.AssignOneTask(); !ok {
		fmt.Println("No agent available")
	} else {
		fmt.Println("Assigned one task", id)
	}
	fmt.Println("Two task assigned", allocate.Printer())

	fmt.Println("---------Three task-----------")
	if id, ok := allocate.AssignOneTask(); !ok {
		fmt.Println("No agent available")
	} else {
		fmt.Println("Assigned one task", id)
	}
	fmt.Println("threee task assigned", allocate.Printer())

	fmt.Println("------------Four task-----------")
	if id, ok := allocate.AssignOneTask(); !ok {
		fmt.Println("No agent available")
	} else {
		fmt.Println("Assigned one task", id)
	}
	fmt.Println("four task assigned", allocate.Printer())

	fmt.Println("------------five task-----------")
	if id, ok := allocate.AssignOneTask(); !ok {
		fmt.Println("No agent available")
	} else {
		fmt.Println("Assigned one task", id)
	}
	fmt.Println("five task assigned", allocate.Printer())
}
