/* Auto-generated config file clock_config.h */
#ifndef CLOCK_CONFIG_H
#define CLOCK_CONFIG_H

// <<< Use Configuration Wizard in Context Menu >>>

#ifndef F_CPU
#define F_CPU 8000000
#endif
// <h> ADC Clock Settings
// <y> ADC Clock source
// <CLKadc"> CLKadc
// <i> This defines the clock source for the ADC module
// <id> adc_clock_source
#define CONF_ADC_SRC CLKadc

// </h>

// <h> TC1 Clock Settings
// <y> TC1 Clock source
// <CLKio"> CLKio
// <i> This defines the clock source for the TC1 module
// <id> tc16_clock_source
#define CONF_TC1_SRC CLKio

// </h>

// <<< end of configuration section >>>

#endif // CLOCK_CONFIG_H
