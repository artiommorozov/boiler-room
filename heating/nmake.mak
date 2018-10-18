OPENSSH=c:\windows\system32\openssh
SCP=$(OPENSSH)\scp.exe
SSH=$(OPENSSH)\ssh.exe

include board.mak

all:
	$(SCP) -Br src $(BOARD_USER)@$(BOARD_IP):/home/$(BOARD_USER)/
	$(SCP) -B Makefile $(BOARD_USER)@$(BOARD_IP):/home/$(BOARD_USER)/
	$(SSH) $(BOARD_USER)@$(BOARD_IP) make
                                                                 
