// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
//
// Copyright 2010-2011 by Wilson Snyder. This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License.
// Version 2.0.
//
// Verilator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//*************************************************************************

#ifdef IS_VPI

#include "vpi_user.h"
#include "stdlib.h"

#else

#include "Vt_vpi_get.h"
#include "verilated.h"
#include "svdpi.h"

#include "Vt_vpi_get__Dpi.h"

#include "verilated_vpi.h"
#include "verilated_vpi.cpp"
#include "verilated_vcd_c.h"

#endif

#include <cstdio>
#include <cstring>
#include <iostream>
using namespace std;

// __FILE__ is too long
#define FILENM "t_vpi_var.cpp"

#define TEST_MSG if (0) printf

unsigned int main_time = false;

//======================================================================


class VlVpiHandle {
    /// For testing, etc, wrap vpiHandle in an auto-releasing class
    vpiHandle m_handle;
public:
    VlVpiHandle() : m_handle(NULL) { }
    VlVpiHandle(vpiHandle h) : m_handle(h) { }
    ~VlVpiHandle() { if (m_handle) { vpi_release_handle(m_handle); m_handle=NULL; } }
    operator vpiHandle () const { return m_handle; }
    inline VlVpiHandle& operator= (vpiHandle h) { m_handle = h; return *this; }
};

//======================================================================

#define CHECK_RESULT_VH(got, exp) \
    if ((got) != (exp)) { \
	printf("%%Error: %s:%d: GOT = %p   EXP = %p\n", \
	       FILENM,__LINE__, (got), (exp)); \
	return __LINE__; \
    }

#define CHECK_RESULT_NZ(got) \
    if (!(got)) { \
	printf("%%Error: %s:%d: GOT = NULL  EXP = !NULL\n", FILENM,__LINE__); \
	return __LINE__; \
    }

// Use cout to avoid issues with %d/%lx etc
#define CHECK_RESULT(got, exp) \
    if ((got) != (exp)) {			     \
	cout<<dec<<"%Error: "<<FILENM<<":"<<__LINE__ \
	   <<": GOT = "<<(got)<<"   EXP = "<<(exp)<<endl;	\
	return __LINE__; \
    }

#define CHECK_RESULT_HEX(got, exp) \
    if ((got) != (exp)) {				  \
	cout<<dec<<"%Error: "<<FILENM<<":"<<__LINE__<<hex \
	   <<": GOT = "<<(got)<<"   EXP = "<<(exp)<<endl;	\
	return __LINE__; \
    }

#define CHECK_RESULT_CSTR(got, exp) \
    if (strcmp((got),(exp))) { \
	printf("%%Error: %s:%d: GOT = '%s'   EXP = '%s'\n", \
	       FILENM,__LINE__, (got)?(got):"<null>", (exp)?(exp):"<null>"); \
	return __LINE__; \
    }

#define CHECK_RESULT_CSTR_STRIP(got, exp) \
    CHECK_RESULT_CSTR(got+strspn(got, " "), exp)

static int _mon_check_props(VlVpiHandle& handle, int size, int direction, int scalar, int type) {
    VlVpiHandle iter_h, left_h, right_h;
    s_vpi_value value = {
      vpiIntVal
    };
    // check size of object
    int vpisize = vpi_get(vpiSize, handle);
    CHECK_RESULT(vpisize, size);
    return 0;
    int vpiscalar = vpi_get(vpiScalar, handle);
    CHECK_RESULT(vpiscalar, scalar);
    int vpivector = vpi_get(vpiVector, handle);
    CHECK_RESULT(vpivector, !scalar);
    if (vpivector) {
      // check coherency for vectors
      // get left hand side of range
      left_h = vpi_handle(vpiLeftRange, handle);
      CHECK_RESULT_NZ(left_h);
      vpi_get_value(left_h, &value);
      int coherency = value.value.integer;
      // get right hand side of range
      right_h = vpi_handle(vpiRightRange, handle);
      CHECK_RESULT_NZ(right_h);
      vpi_get_value(right_h, &value);
      printf("%d:%d\n",coherency, value.value.integer); 
      coherency -= value.value.integer;
      // calculate size & check
      coherency = abs(coherency) + 1;
      CHECK_RESULT(coherency, size);
    }

    // check direction of object
    int vpidir = vpi_get(vpiDirection, handle);
    CHECK_RESULT(vpidir, direction);

    // check type of object
    int vpitype = vpi_get(vpiType, handle);
    CHECK_RESULT(vpitype, type);

    return 0; // Ok
}

int mon_check_props() {
    VlVpiHandle onebit_h = vpi_handle_by_name((PLI_BYTE8*)"t.onebit", NULL);
      CHECK_RESULT_NZ(onebit_h);
    if (int status = _mon_check_props(onebit_h, 1, vpiNoDirection, 1, vpiReg)) return status;
    VlVpiHandle twoone_h = vpi_handle_by_name((PLI_BYTE8*)"t.twoone", NULL);
    if (int status = _mon_check_props(twoone_h, 2, vpiNoDirection, 0, vpiReg)) return status;
    VlVpiHandle fourthreetwoone_h = vpi_handle_by_name((PLI_BYTE8*)"t.fourthreetwoone", NULL);
    if (int status = _mon_check_props(fourthreetwoone_h, 2, vpiNoDirection, 0, vpiMemory)) return status;
    VlVpiHandle iter_h = vpi_iterate(vpiMemoryWord, fourthreetwoone_h);
    while (VlVpiHandle word_h = vpi_scan(iter_h)) {
      // check size and range
      if (int status = _mon_check_props(word_h, 2, vpiNoDirection, 0, vpiMemoryWord)) return status;
    }
      printf("clk\n"); 
    VlVpiHandle clk_h = vpi_handle_by_name((PLI_BYTE8*)"t.clk", NULL);
      if (int status = _mon_check_props(clk_h, 1, vpiInput, 1, vpiReg)) return status;
      printf("quads0\n"); 
    VlVpiHandle quads0_h = vpi_handle_by_name((PLI_BYTE8*)"t.quads0", NULL);
    if (int status = _mon_check_props(quads0_h, 2, vpiInput, 0, vpiMemory)) return status;
    return 0;
}

int mon_check() {
    // Callback from initial block in monitor
    if (int status = mon_check_props()) return status;
    return 0; // Ok
}

//======================================================================

#ifdef IS_VPI

static s_vpi_systf_data vpi_systf_data[] = {
  {vpiSysFunc, vpiSysFunc, (PLI_BYTE8*)"$mon_check", (PLI_INT32(*)(PLI_BYTE8*))mon_check, 0, 0, 0},
  0
};

// cver entry
void vpi_compat_bootstrap(void) {
  p_vpi_systf_data systf_data_p;
  systf_data_p = &(vpi_systf_data[0]);
  while (systf_data_p->type != 0) vpi_register_systf(systf_data_p++);
}

// icarus entry
void (*vlog_startup_routines[])() = {
      vpi_compat_bootstrap,
      0
};

#else
double sc_time_stamp () {
    return main_time;
}
int main(int argc, char **argv, char **env) {
    double sim_time = 1100;
    Verilated::commandArgs(argc, argv);
    Verilated::debug(0);

    VM_PREFIX* topp = new VM_PREFIX ("");  // Note null name - we're flattening it out

#ifdef VERILATOR
# ifdef TEST_VERBOSE
    Verilated::scopesDump();
# endif
#endif

    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
#if VM_TRACE
    VL_PRINTF("Enabling waves...\n");
    topp->trace (tfp, 99);
    tfp->open ("obj_dir/t_vpi_var/simx.vcd");
#endif

    topp->eval();
    topp->clk = 0;
    main_time += 10;

    while (sc_time_stamp() < sim_time && !Verilated::gotFinish()) {
	main_time += 1;
	topp->eval();
	VerilatedVpi::callValueCbs();
	topp->clk = !topp->clk;
	//mon_do();
#if VM_TRACE
	if (tfp) tfp->dump (main_time);
#endif
    }
    if (!Verilated::gotFinish()) {
	vl_fatal(FILENM,__LINE__,"main", "%Error: Timeout; never got a $finish");
    }
    topp->final();

#if VM_TRACE
    if (tfp) tfp->close();
#endif

    delete topp; topp=NULL;
    exit(0L);
}

#endif
