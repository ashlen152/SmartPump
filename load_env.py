import os
# Temporarily commented out for compilation check
#from dotenv import load_dotenv
#load_dotenv()

def before_build(env, platform):
    # Read environment variables from .env
    wifi_ssid = os.getenv("WIFI_SSID", "DefaultSSID")
    wifi_password = os.getenv("WIFI_PASSWORD", "DefaultPassword")
    server_address = os.getenv("SERVER_ADDRESS", "192.168.68.108")
    server_port = os.getenv("SERVER_PORT", "3000")

    # Pass them as build flags
    env.Append(
        CPPDEFINES=[
            ("WIFI_SSID", f'"{wifi_ssid}"'),
            ("WIFI_PASSWORD", f'"{wifi_password}"'),
            ("SERVER_ADDRESS", f'"{server_address}"'),
            ("SERVER_PORT", server_port)
        ]
    )