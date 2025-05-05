# ───────────────────[ Top‑level Makefile ]───────────────────
# 기본 컴파일러 / 플래그
CC      := gcc
CFLAGS  := -g -O0 -Wall -Wextra -fno-omit-frame-pointer
LDFLAGS :=

# ───────────── echo 디렉터리 공통 정의 ─────────────
ECHO_DIR       := echo
ECHO_COMMON_SRCS := $(ECHO_DIR)/csapp.c
ECHO_COMMON_OBJS := $(ECHO_COMMON_SRCS:.c=.o)

# (1) echoserveri
ES_NAME        := echoserveri
ES_SRCS        := $(ECHO_DIR)/$(ES_NAME).c
ES_OBJS        := $(ES_SRCS:.c=.o) $(ECHO_COMMON_OBJS)

# (2) echoclient
EC_NAME        := echoclient
EC_SRCS        := $(ECHO_DIR)/$(EC_NAME).c
EC_OBJS        := $(EC_SRCS:.c=.o) $(ECHO_COMMON_OBJS)

# (3) hostinfo
HI_NAME        := hostinfo
HI_SRCS        := $(ECHO_DIR)/$(HI_NAME).c
HI_OBJS        := $(HI_SRCS:.c=.o) $(ECHO_COMMON_OBJS)

# ───────────── webproxy‑lab (proxy 서버) ─────────────
PROXY_DIR      := webproxy-lab        # 이미 자체 Makefile 존재
PROXY_TARGET   := proxy               # webproxy-lab/Makefile이 생성하는 실행파일 이름

# ───────────── tiny 서버 (sub‑Makefile) ─────────────
TINY_DIR       := $(PROXY_DIR)/tiny   # CS:APP tiny 서버 실습

# ───────────── 최상위 타깃 ─────────────
.PHONY: all echo proxy tiny clean
all: echo proxy tiny

# ───────────── echo 그룹 빌드 ─────────────
.PHONY: echo
echo: $(ES_NAME) $(EC_NAME) $(HI_NAME)

$(ES_NAME): $(ES_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(EC_NAME): $(EC_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(HI_NAME): $(HI_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# 공통 오브젝트 규칙
$(ECHO_DIR)/%.o: $(ECHO_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ───────────── proxy & tiny는 서브‑Make 호출 ─────────────
.PHONY: proxy
proxy:
	$(MAKE) -C $(PROXY_DIR)

.PHONY: tiny
tiny:
	$(MAKE) -C $(TINY_DIR)

# ───────────── 클린업 ─────────────
.PHONY: clean
clean:
	@echo "Cleaning echo objects/binaries…"
	@rm -f $(ECHO_DIR)/*.o $(ES_NAME) $(EC_NAME) $(HI_NAME)
	@echo "Cleaning proxy & tiny…"
	@$(MAKE) -C $(PROXY_DIR) clean
	@$(MAKE) -C $(TINY_DIR)  clean