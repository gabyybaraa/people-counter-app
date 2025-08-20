@echo off
echo Starting iOS Simulator build...
echo.
echo This will run your Flutter app on iOS Simulator without code signing issues.
echo.
cd /d "%~dp0"
flutter clean
flutter pub get
flutter run -d ios
pause
