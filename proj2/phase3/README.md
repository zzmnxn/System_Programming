
### 기본 명령 실행
- `ls`, `mkdir`, `rmdir`, `touch`, `cat`, `echo`, `cd`, `exit` 등의 기본 명령어 실행 지원
- `cd`, `exit`은 부모 프로세스에서 직접 처리됨

### 파이프(`|`) 처리
- `cat file | grep word | sort` 형태의 파이프라인 명령어 처리 가능
- 각 명령어는 별도의 자식 프로세스로 실행되고 `dup2()`로 입출력 연결됨

### 백그라운드 실행 (`&`)
- 명령어 끝에 `&`가 있는 경우 백그라운드에서 실행됨
- `sleep 10 &`, `cat file | grep hello &` 등 파이프 조합도 가능
- `sleep&` 형태처럼 공백 없이 입력해도 정상 처리됨

### Job Control 기능
- `jobs`: 현재 실행 중이거나 멈춘(background/stopped) job 출력
- `fg <job_id>`: 해당 job을 포그라운드로 가져오기 
- `bg <job_id>`: 멈춘 job을 백그라운드에서 재실행
- `kill %<job_id>`: 해당 job 강제 종료 (% 사용 필수)

### 시그널 핸들링
- `Ctrl+C` (`SIGINT`): foreground 프로세스를 종료
- `Ctrl+Z` (`SIGTSTP`): foreground 프로세스를 멈춤 (Stopped 상태로 전환)
- `SIGCHLD`: 자식 종료 시 job table에서 제거
- 터미널 제어: `tcsetpgrp`로 fg job에 터미널 제어권 부여/회수

## 사용 예시

```bash
# 기본 명령
ls
mkdir test_dir
cd test_dir

# 파이프 사용
echo hello world | grep hello

# 백그라운드 실행
sleep 5 &
cat myshell.c | grep -i main &

# job 제어
jobs
fg 1
bg 1
kill %1
