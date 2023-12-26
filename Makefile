PROJECT_NAME := SPIPeripheral
VERILATOR_OUT := verilated
SIM_OUT := simbuild

VERILATOR_INCLUDE := /opt/homebrew/share/verilator/include

CXX = clang++
CXX_FLAGS = --std=c++17
CXX_FLAGS += -I${VERILATOR_INCLUDE}
CXX_FLAGS += -I${VERILATOR_OUT}/
CXX_FLAGS += ${VERILATOR_INCLUDE}/verilated.cpp
CXX_FLAGS += ${VERILATOR_INCLUDE}/verilated_vcd_c.cpp
CXX_FLAGS += sim/${PROJECT_NAME}_sim.cpp
CXX_FLAGS += ${VERILATOR_OUT}/V${PROJECT_NAME}__ALL.a
CXX_FLAGS += -o ${SIM_OUT}/${PROJECT_NAME}

.PHONY: all clean lint
all: buildsim

verilate:
	@echo "** Verilating RTL"
	@verilator -f ${PROJECT_NAME}.vflags -Mdir ${VERILATOR_OUT} --trace

buildsim:
	@echo "** Building c++ files"
	@mkdir -p ${SIM_OUT}
	@cd ${VERILATOR_OUT} && ${make} -f V${PROJECT_NAME}.mk
	@${CXX} ${CXX_FLAGS}

sim: buildsim
	@echo "** Running ${PROJECT_NAME} simulation"
	@${SIM_OUT}/${PROJECT_NAME}

view:
	@echo "** View inferred design"
	@yosys -s view.ys

lint:
	@verilator --lint-only -f ${PROJECT_NAME}.vflags

clean:
	@echo "** Cleaning up workspace"
	@rm -rf ${VERILATOR_OUT}
	@rm -rf ${SIM_OUT}
	@rm *.vcd

