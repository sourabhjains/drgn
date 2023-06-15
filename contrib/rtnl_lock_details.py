import ctypes

def print_task_details(pid, task, cpu, comm):
        print("PID: %d  TASK: %s  CPU: %d  COMMAND: %s" % (pid, task, cpu, comm))

def print_rtnl_lock_details():
    rtnl_mutex_obj = prog["rtnl_mutex"]
    owner = rtnl_mutex_obj.owner
    owner_task_add = hex(ctypes.c_ulong(owner.counter).value)
    owner_task = cast("struct task_struct *", rtnl_mutex_obj.owner.counter)
    print("Current task with an active lock:")
    print_task_details(owner_task.pid, owner_task_add,
                       owner_task.cpu, owner_task.comm)
    head = rtnl_mutex_obj.wait_list.address_of_()
    print("\nList of waiter for RTNL Mutex lock:")

    # Loop though the list and print the lock waiter
    node = head.next
    while node != head:
        waiter = container_of(node, "struct mutex_waiter", "list")
        task_addr = hex(node)
        print_task_details(waiter.task.pid, task_addr,
                           waiter.task.cpu, waiter.task.comm)
        node = node.next

print_rtnl_lock_details()
