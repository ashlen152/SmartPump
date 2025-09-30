import os
# Temporarily commented out for compilation check
#from dotenv import load_dotenv
#load_dotenv()

def before_build(env, platform):
    # Read environment variables from .env
    wifi_ssid = os.getenv("WIFI_SSID", "DefaultSSID")
    wifi_password = os.getenv("WIFI_PASSWORD", "DefaultPassword")

    # Pass them as build flags
    env.Append(
        CPPDEFINES=[
            ("WIFI_SSID", f'"{wifi_ssid}"'),
            ("WIFI_PASSWORD", f'"{wifi_password}"')
        ]
    )