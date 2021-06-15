set JAVA_HOME=C:\Program Files\Java\jdk1.8.0_241
echo %JAVA_HOME%
cmd.exe /c gradlew.bat assembleHeadlessDebug assembleGalleryDebug
