
### �⺻ ��� ����
- `ls`, `mkdir`, `rmdir`, `touch`, `cat`, `echo`, `cd`, `exit` ���� �⺻ ��ɾ� ���� ����
- `cd`, `exit`�� �θ� ���μ������� ���� ó����

### ������(`|`) ó��
- `cat file | grep word | sort` ������ ���������� ��ɾ� ó�� ����
- �� ��ɾ�� ������ �ڽ� ���μ����� ����ǰ� `dup2()`�� ����� �����

### ��׶��� ���� (`&`)
- ��ɾ� ���� `&`�� �ִ� ��� ��׶��忡�� �����
- `sleep 10 &`, `cat file | grep hello &` �� ������ ���յ� ����
- `sleep&` ����ó�� ���� ���� �Է��ص� ���� ó����

### Job Control ���
- `jobs`: ���� ���� ���̰ų� ����(background/stopped) job ���
- `fg <job_id>`: �ش� job�� ���׶���� �������� 
- `bg <job_id>`: ���� job�� ��׶��忡�� �����
- `kill %<job_id>`: �ش� job ���� ���� (% ��� �ʼ�)

### �ñ׳� �ڵ鸵
- `Ctrl+C` (`SIGINT`): foreground ���μ����� ����
- `Ctrl+Z` (`SIGTSTP`): foreground ���μ����� ���� (Stopped ���·� ��ȯ)
- `SIGCHLD`: �ڽ� ���� �� job table���� ����
- �͹̳� ����: `tcsetpgrp`�� fg job�� �͹̳� ����� �ο�/ȸ��

## ��� ����

```bash
# �⺻ ���
ls
mkdir test_dir
cd test_dir

# ������ ���
echo hello world | grep hello

# ��׶��� ����
sleep 5 &
cat myshell.c | grep -i main &

# job ����
jobs
fg 1
bg 1
kill %1
