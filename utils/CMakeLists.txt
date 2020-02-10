# Install systemd files for server if running on Linux
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    configure_file(tpm_server.service.in tpm_server.service @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tpm_server.service
        DESTINATION /lib/systemd/system )
endif()
