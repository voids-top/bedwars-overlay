cl /EHsc /O2 /utf-8 injector.cpp
cl /LD /utf-8 /EHsc util.cpp /I"%JAVA_HOME%\include" /I"%JAVA_HOME%\include\win32" Ws2_32.lib "%JAVA_HOME%\lib\jvm.lib"
pause