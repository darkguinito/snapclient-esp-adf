
idf_component_register(SRCS "sample_format.c" "snapclient_stream.c"
                       INCLUDE_DIRS "include"
                       REQUIRES audio_pipeline audio_sal esp-adf-libs lightsnapcast tcp_transport audio_hal)
