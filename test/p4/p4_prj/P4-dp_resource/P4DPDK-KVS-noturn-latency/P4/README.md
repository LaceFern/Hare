# P4-Lock

## Module Structure

```
P4Lock
├── Ingress
│   ├── control
│   │   └── control_flag
│   │       ├── statistic_flag
│   │       └── suspend_flag
│   └── precise_counter
└── Egress
    ├── pre_queue_info
    │   ├── queue_counter
    │   ├── queue_head
    │   └── queue_tail
    ├── lock_queue_node
    │   └── lock_queue
    └── forward
```