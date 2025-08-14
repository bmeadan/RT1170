# Rotem-RT1170

## Environment setup

Add MCUX environment variables to your shell profile.


#### Linux:
```bash
export MCUX_SDK=$HOME/mcux_sdk
export MCUX_TOOLCHAIN=$HOME/.mcuxpressotools/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi
```

#### Windows:
```cmd
setx MCUX_SDK "%USERPROFILE%\mcux_sdk"
setx MCUX_TOOLCHAIN "%USERPROFILE%\.mcuxpressotools\arm-gnu-toolchain-13.2.Rel1-mingw-w64-i686-arm-none-eabi"
```

#### Macos:
```bash
export MCUX_SDK=$HOME/mcuxpresso_sdks
export MCUX_TOOLCHAIN=/Applications/MCUXpressoIDE_24.9.25/ide/tools
```