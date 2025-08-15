# A driver for an EC11 encoder using the peripheral: PCNT

(ESP32, esp-idf)

This is a lighter version from: 
https://github.com/espressif/esp-idf/tree/master/examples/peripherals/pcnt/rotary_encoder

- Uses only one pcnt channel = channel a

### `Supported Targets:	ESP32, ESP32-C5, ESP32-C6, ESP32-H2, ESP32-P4, ESP32-S2, ESP32-S3`

>
    A      +-----+     +-----+     +-----+
                 |     |     |     |
                 |     |     |     |
                 +-----+     +-----+
    B         +-----+     +-----+     +-----+
                    |     |     |     |
                    |     |     |     |
                    +-----+     +-----+

     +--------------------------------------->
                CW direction : falling edge on A before falling edge on B

>

>
    A         +-----+     +-----+     +-----+
                    |     |     |     |
                    |     |     |     |
                    +-----+     +-----+
    B      +-----+     +-----+     +-----+
                 |     |     |     |
                 |     |     |     |
                 +-----+     +-----+

     +--------------------------------------->
                CCW direction : falling edge on B before falling edge on A

>