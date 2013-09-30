class simulator {
 private :
   typedef struct {
     int verilator;
     int icarus;
     int mti;
     int ncsim;
     int vcs;
   } sim_types;
   s_vpi_vlog_info   info;
   sim_types         simulators;
 public :
   static simulator* self;
   simulator() {
     vpi_get_vlog_info(&info);
     simulators.verilator = strcmp(info.product, "Verilator") == 0;
     simulators.icarus = strcmp(info.product, "Icarus Verilog") == 0;
   }
   ~simulator();
   static simulator& instance() {
     if (self == NULL) self = new simulator();
     return *self;
   }
   s_vpi_vlog_info& get_info() {
     return info;
   }
   sim_types& get() {
    return simulators;
   }
   bool is_event_driven() {
     return !simulators.verilator;
   }
   bool is_free() {
     return simulators.verilator || simulators.icarus;
   }
   bool has_get_scalar() {
     return !simulators.icarus;
   }
   // return test level scope
   const char *top() {
      if (simulators.verilator) {
        return "t";
      } else {
        return "top.t";
      }
   }
   // return absolute scope of obj
   const char* rooted(const char *obj) {
     static char buf[256];
     snprintf(buf, sizeof(buf), "%s.%s", top(), obj);
     return buf;
  }
};

simulator *simulator::self = NULL;

#define VPI_HANDLE(signal) vpi_handle_by_name((PLI_BYTE8*)simulator::instance().rooted(signal), NULL);
