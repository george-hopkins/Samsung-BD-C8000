#=============================================================
#             Target Definition
# make --> build & clean
# 
# make X260         --> X260 build
# make A1	    --> A1 build
# 
# make cX260        --> X260 clean
# make cA1	    --> A1 clean
#=============================================================


ARG1 = $(shell echo $(MAKECMDGOALS) | awk  '{print $$1}')
ARG2 = $(shell echo $(MAKECMDGOALS) | awk  '{print $$2}')


ifeq ($(ARG1),X260)
CORE=X260
endif
ifeq ($(ARG1),cX260)
CORE=X260
endif

ifeq ($(ARG1),A1)
CORE=A1
endif
ifeq ($(ARG1),cA1)
CORE=A1
endif

ifeq ($(ARG1),STI)
CORE=STI
endif
ifeq ($(ARG1),cSTI)
CORE=STI
endif

export CORE



all:   
	./makeAllLib.sh
clean:    
	./cleanAllLib.sh


X260 :		
	@echo "---------------------------------------------------------"
	@echo "|          Build smpeg for X260   	    		|"
	@echo "---------------------------------------------------------"
	$(MAKE) -f Makefile.X260 all
	@echo "---------------------------------------------------------"
	@echo "|          Done smpeg for X260   	    		|"
	@echo "---------------------------------------------------------"

cX260 :
	@echo "---------------------------------------------------------"
	@echo "|          Clean & uninstall smpeg for X260  	  	|"
	@echo "---------------------------------------------------------"
	$(MAKE) clean -f Makefile.X260 clean  

A1 :		
	@echo "---------------------------------------------------------"
	@echo "|          Build smpeg for A1   			  	|"
	@echo "---------------------------------------------------------"
	$(MAKE) -f Makefile.A1 all
	@echo "---------------------------------------------------------"
	@echo "|          Done smpeg for X260   	    		|"
	@echo "---------------------------------------------------------"

	
cA1 :
	@echo "---------------------------------------------------------"
	@echo "|          Clean & uninstall smpeg for A1    		|"
	@echo "---------------------------------------------------------"
	$(MAKE) clean -f Makefile.A1 clean  




STI :		
	@echo "---------------------------------------------------------"
	@echo "|          Build smpeg for STI   		    		|"
	@echo "---------------------------------------------------------"
	$(MAKE) -f Makefile.STI all
	@echo "---------------------------------------------------------"
	@echo "|          Build Done smpeg for STI			|"
	@echo "---------------------------------------------------------"

	
cSTI :
	@echo "---------------------------------------------------------"
	@echo "|          Clean & uninstall smpeg for STI    		|"
	@echo "---------------------------------------------------------"
	$(MAKE) clean -f Makefile.STI clean  


