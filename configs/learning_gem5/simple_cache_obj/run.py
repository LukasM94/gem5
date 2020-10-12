import m5
from m5.objects import *

# Set the clock fequency of the system (and all of its children)
system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

# Set up the system
system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('1024MB')]

# Create TimingSimpleCPU
system.cpu = TimingSimpleCPU()

# Create SimpleMemObj
system.memobj = SimpleMemObj()

# Create SimpleCache
system.cache = SimpleCacheObj(size='1kB')

# Bind cpu instruction and data cache to the SimpleMemObj
# instruction and data ports
system.cpu.icache_port = system.cache.cpu_ports
system.cpu.dcache_port = system.cache.cpu_ports

# Create a memory bus, a coherent crossbar, in this case
system.membus = SystemXBar()

# Bind SimpleMemObj memory port to the memory bus
system.cache.mem_port = system.membus.slave

# Create InterruptController
system.cpu.createInterruptController()
system.cpu.interrupts[0].pio = system.membus.master
system.cpu.interrupts[0].int_master = system.membus.slave
system.cpu.interrupts[0].int_slave = system.membus.master

# Create MemCtrl
system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.master

# Connect the system up to the membus
system.system_port = system.membus.slave

# Create Process
process = Process()
process.cmd = ['tests/test-progs/hello/bin/x86/linux/hello']
# process.cmd = ['tests/test-progs/large_array/a.out']

system.cpu.workload = process

# Create Threads
system.cpu.createThreads()

root = Root(full_system = False, system = system)
m5.instantiate()

print('Beginning simulation!')
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'.format(m5.curTick(),
                                  exit_event.getCause()))