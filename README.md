An experiment to create a stack for STM32 that is limited to using only a snake_case.

Additional practical purpose: evaluation project to test Ethernet protocol for XRD cell.

Hardware (board) 32F746GDISCOVERY MB1191 B-04.

Clone repo with submodules!
`git clone --recurse-submodules git@github.com:dzusan/snake_stack.git`

# Build

1. `mkdir bin`
2. `cmake . -B bin`
3. `cd bin`
4. `make`


# Dependencies

* [libopencm3](https://github.com/libopencm3/libopencm3)
* [threadx](https://github.com/azure-rtos/threadx.git)
* [lwip](https://git.savannah.gnu.org/git/lwip.git)


# References

* [stm32f107_ethernet_test](https://github.com/berndoJ/stm32f107_ethernet_test)
* [ST UM: Developing applications on STM32Cube with LwIP TCP/IP stack](https://www.st.com/resource/en/user_manual/um1713-developing-applications-on-stm32cube-with-lwip-tcpip-stack-stmicroelectronics.pdf)
* [Networking services plugin](https://github.com/grblHAL/Plugin_networking)
* [barebox](https://github.com/saschahauer/barebox)
* [frosted](https://github.com/insane-adding-machines/frosted)
