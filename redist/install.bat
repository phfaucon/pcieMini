@ECHO OFF
cd %~dp0
wdreg -inf windrvr1440.inf install
if exist Alphi508.inf (
  wdreg -inf Alphi508.inf install
)

