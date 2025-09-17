ENTRY(_start)

SECTIONS {
  . = ORIGIN(ram);
  .text : ALIGN(4) {
      KEEP(*(entry))
      . = ALIGN(4);
      __gap_begin = .;
      . = . + 0x2000;
      __gap_end = .;
      *(.text .text.*)
    } > ram
  etext = .;
  _etext = .;
  .rodata : {
    *(.rodata*)
  }
  .data : {
    *(.data)
  }
  edata = .;
  _data = .;
  .bss : {
	_bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  }
  _stack_top = ALIGN(0x1000);
  . = _stack_top + 0x8000;
  _stack_pointer = .;
  . = _stack_top + 0x320000;
  end = .;
  _end = .;
  _heap_start = ALIGN(0x1000);
  _pmem_start = pmem_base;
  _pmem_end = _pmem_start + LENGTH(ram);
}
