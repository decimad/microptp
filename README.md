# microptp

c++ implementation of a non-conforming ptp clock (slave only for now)
- sends a delay req per sync (since I don't know if delay reqs need to be statistically distributed)
- no master timeout removal (since I don't know the specs for this to conform)
- see file microptp/ports/systemport.hpp for the system port api, see included port onethread for a sample implementation using stmlib on a stm32f407
- feel free to donate a copy of the spec ;)

memory footprint
  - text: 40.6 kb including ethernet driver, lwip with dhcp and the stmlib ptp port. (O3, lto, arm gcc 4.9)
  - bss : 48 kb including 25kb lwip heap+buffers, 8kb stmlib rx buffers, 4kb net thread stack (can be tuned down...)


relies on these libraries
- microlib (https://github.com/decimad/microlib)
- fixed point lib (https://github.com/decimad/fixed)
- (stmlib (https://github.com/decimad/stmlib) for the two included ports)
