#include <stdbool.h>

static const int air_conditioning_factor = -1;
static const int heating_factor = 1;
static const int ventilation_multiplier = 2;

typedef struct {
   bool air_conditioning;
   bool heating;
   bool ventilation;
} thermostat_status;