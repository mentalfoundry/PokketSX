set LUPDATE=..\..\dep\msvc\qt\6.1.0\msvc2019_64\bin\lupdate.exe ./ ../core/ ../frontend-common/ -tr-function-alias translate+=TranslateString -tr-function-alias translate+=TranslateStdString -tr-function-alias QT_TRANSLATE_NOOP+=TRANSLATABLE

%LUPDATE% -ts translations\pokketsx-qt_de.ts
%LUPDATE% -ts translations\pokketsx-qt_es.ts
%LUPDATE% -ts translations\pokketsx-qt_fr.ts
%LUPDATE% -ts translations\pokketsx-qt_he.ts
%LUPDATE% -ts translations\pokketsx-qt_it.ts
%LUPDATE% -ts translations\pokketsx-qt_ja.ts
%LUPDATE% -ts translations\pokketsx-qt_nl.ts
%LUPDATE% -ts translations\pokketsx-qt_pl.ts
%LUPDATE% -ts translations\pokketsx-qt_pt-br.ts
%LUPDATE% -ts translations\pokketsx-qt_pt-pt.ts
%LUPDATE% -ts translations\pokketsx-qt_ru.ts
%LUPDATE% -ts translations\pokketsx-qt_tr.ts
%LUPDATE% -ts translations\pokketsx-qt_zh-cn.ts
pause
