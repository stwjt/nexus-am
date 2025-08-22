include $(AM_HOME)/Makefile.check
.PHONY: all clean $(ALL)

ALL = direct_jump_chain jump_call_pattern call_ret_pattern pt_2nd_false invalid_combinations edge_cases performance_test interaction_test quick_test

all: $(ALL)

$(ALL): %:
	@mkdir -p build/$*
	@echo "NAME := $*" > build/$*/Makefile
	@echo "SRCS := ../../$*.c" >> build/$*/Makefile  
	@echo "LIBS += klib" >> build/$*/Makefile
	@echo "include \$$(AM_HOME)/Makefile.app" >> build/$*/Makefile
	@$(MAKE) -C build/$* ARCH=$(ARCH)

clean:
	rm -rf build/

#cancel rules included by $(AM_HOME)/Makefile.check  
image: ;
run: ; 