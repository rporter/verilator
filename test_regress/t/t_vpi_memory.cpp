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

#else

#include "Vt_vpi_memory.h"
#include "verilated.h"
#include "svdpi.h"

#include "Vt_vpi_memory__Dpi.h"

#include "verilated_vpi.h"
#include "verilated_vpi.cpp"
#include "verilated_vcd_c.h"

#endif

#include <stdio.h>
#include <string.h>
#include <iostream>
using namespace std;

// __FILE__ is too long
#define FILENM "t_vpi_memory.cpp"

#define DEBUG if (0) printf

unsigned int main_time = false;

//======================================================================

class VlVpiHandle {
    /// For testing, etc, wrap vpiHandle in an auto-releasing class
    vpiHandle m_handle;
public:
    VlVpiHandle() : m_handle(NULL) { }
    VlVpiHandle(vpiHandle h) : m_handle(h) { }
    ~VlVpiHandle() { if (m_handle) { vpi_free_object(m_handle); m_handle=NULL; } } // icarus has yet to catch up with 1800-2009
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
    if ((got != exp)) { \
	cout<<dec<<"%Error: "<<FILENM<<":"<<__LINE__ \
	   <<": GOT = "<<(got)<<"   EXP = "<<(exp)<<endl;	\
	return __LINE__; \
    }

#define CHECK_RESULT_HEX(got, exp) \
    if ((got != exp)) { \
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

// ideally we should be able to iterate on vpiRange against a list of this struct
typedef struct range {
    int size;
    int left;
    int right;
} range_s, *range_p;

int _mon_check_range(VlVpiHandle& handle, int size, int left, int right) {
    VlVpiHandle iter_h, rng_h, left_h, right_h;
    s_vpi_value value = {
      vpiIntVal
    };
    // check size of object
    int vpisize = vpi_get(vpiSize, handle);
    CHECK_RESULT(vpisize, size);
/*
    // get range and check against expected
    iter_h = vpi_iterate(vpiRange, handle);
    CHECK_RESULT_NZ(iter_h);
    rng_h = vpi_scan(iter_h);
    CHECK_RESULT_NZ(rng_h);
    int vpitype = vpi_get(vpiType, rng_h);
    CHECK_RESULT(vpitype, vpiRange);
*/
    // check size of range
    vpisize = vpi_get(vpiSize, handle);
    CHECK_RESULT(vpisize, size);
    // check left hand side of range
    left_h = vpi_handle(vpiLeftRange, handle);
    CHECK_RESULT_NZ(left_h);
    vpi_get_value(left_h, &value);
    CHECK_RESULT(value.value.integer, left);
    // check right hand side of range
    right_h = vpi_handle(vpiRightRange, handle);
    CHECK_RESULT_NZ(right_h);
    vpi_get_value(right_h, &value);
    CHECK_RESULT(value.value.integer, right);
    return 0; // Ok
}

int _mon_check_memory() {
    int cnt;
    VlVpiHandle mem_h, lcl_h;
    vpiHandle iter_h; // icarus does not like auto free of iterator handles
    s_vpi_value value = {
      vpiIntVal
    };
    vpi_printf((PLI_BYTE8*)"Check memory vpi ...\n");
    mem_h = vpi_handle_by_name((PLI_BYTE8*)"t.mem0", NULL);
    CHECK_RESULT_NZ(mem_h);
    // check type
    int vpitype = vpi_get(vpiType, mem_h);
    CHECK_RESULT(vpitype, vpiMemory);
    _mon_check_range(mem_h, 16, 16, 1);
    // iterate and store
    iter_h = vpi_iterate(vpiMemoryWord, mem_h);
    cnt = 0;
    while (lcl_h = vpi_scan(iter_h)) {
	value.value.integer = ++cnt;
        vpi_put_value(lcl_h, &value, NULL, vpiNoDelay);
        // check size and range
        _mon_check_range(lcl_h, 32, 31, 0);
    }
    CHECK_RESULT(cnt, 16); // should be 16 addresses
    // iterate and accumulate
    iter_h = vpi_iterate(vpiMemoryWord, mem_h);
    cnt = 0;
    while (lcl_h = vpi_scan(iter_h)) {
      ++cnt;
      vpi_get_value(lcl_h, &value);
      CHECK_RESULT(value.value.integer, cnt);
    }
    CHECK_RESULT(cnt, 16); // should be 16 addresses
    // don't care for non verilator
    // (crashes on Icarus)
    s_vpi_vlog_info info;
    vpi_get_vlog_info(&info);
    if (strcmp(info.product, "Verilator") != 0) {
	vpi_printf((PLI_BYTE8*)"Skipping property checks for simulator %s\n", info.product);
        return 0; // Ok
    }
    // make sure trying to get properties that don't exist
    // doesn't crash
    int should_be_0 = vpi_get(vpiSize, iter_h);
    CHECK_RESULT(should_be_0, 0);
    should_be_0 = vpi_get(vpiIndex, iter_h);
    CHECK_RESULT(should_be_0, 0);
    vpiHandle should_be_NULL = vpi_handle(vpiLeftRange, iter_h);
    CHECK_RESULT(should_be_NULL, 0);
    should_be_NULL = vpi_handle(vpiRightRange, iter_h);
    CHECK_RESULT(should_be_NULL, 0);
    should_be_NULL = vpi_handle(vpiScope, iter_h);
    CHECK_RESULT(should_be_NULL, 0);
    return 0; // Ok
}

int mon_check() {
    // Callback from initial block in monitor
    if (int status = _mon_check_memory()) return status;
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
    Verilated::fatalOnVpiError(0); // we're going to be checking for these errors do don't crash out

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

