# database-system

CSED421-01 과목 내 프로젝트를 위한 repository

## 개발환경 구축
- Ubuntu 20.04 LTS on Windows
- WSL(Windows Subsystem for Linux)
   - WSL v2를 설치하는 과정에서 윈도우 바이오스 버전과 호환 이슈<br/>
     (https://codefellows.github.io/setup-guide/windows/)
- VS Code
   - 설치한 Extensions: C/C++, Remote WSL)
   - C/C++의 경우 compiler path를 설정해주어야 한다. (in c_cpp_properties.json in the .vscode folder)
- C++ 사용을 위한 컴파일 및 빌드를 위한 모듈(`sudo apt-get install build-essential gdb`)

## 학습 목록
- gitignore 작성법 (bin/*.out)
