/*
 *  linux/arch/arm/oprofile/aop_demangle.c
 *
 *  ELF Symbol demangler Module
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-09-14  Created by gaurav.j@samsung.com
 *
 */
#include <linux/kernel.h>
#include <linux/string.h>

#define atoi(str) simple_strtoul(((str != NULL) ? str : ""), NULL, 0)


/*
* This will copy the src buff to dest. buff for a specified len
* and also chk the new_buff wont be exceeded to the given length
*/

#define AOP_COPY_BUFF_TEST(dest, src, len) \
	{\
		if((cur_len + len)>= buf_len){\
			new_buff[cur_len]  = '\0';\
			return 0;\
		}\
		strncpy(&new_buff[cur_len], src, len);\
		cur_len += len;	\
	}

/*
* This will copy the src buff to dest. buff up to the source len 
*and also increment the buff with the match len and also chk 
* the new_buff wont be exceeded to the given length
*/

#define AOP_COPY_TEST(dest, src, match_str_len) \
	{\
		temp_len = (int)strlen(src);\
		if((cur_len + temp_len)>= buf_len){\
			new_buff[cur_len]  = '\0';\
			return 0;\
		} \
		strcpy(&new_buff[cur_len], src);\
		cur_len += temp_len;\
		buff += match_str_len;\
	}			

/*
* Return the demangled ident length
*/

static int aop_sym_demangled_ident_len(char *buff)
{
	int func_len = 0;
	char num[3] = {0,};

	 if (buff == NULL || *buff == '\0')
 		return -1;

	if(buff[0] >= '0' || buff[0] <= '9'){
		num[0] = buff[0];
		if(buff[1] >= '0' || buff[1] <= '9'){
			num[1] = buff[1];
			num[2] = '\0';
		}
		else	{
			num[1] = '\0';
		}

		func_len = atoi(num);
		return func_len;
	}
	else
		return -1;	
}

/*
* mangled the demangled symbol
*/

int aop_sym_demangle(char* buff, char* new_buff, int buf_len)
{
	int len = 0;
	int cur_len =0; 
	int temp_len = 0;
	int temp_id_len = 0;

	 if (buff == NULL || *buff == '\0'){
	 	return 0;
	 }

	if(!strncmp(buff, "_GLOBAL__I_", strlen("_GLOBAL__I_"))){
		AOP_COPY_TEST(new_buff, "global constructors keyed to ", strlen("_GLOBAL__I_"));
	}

	if(*buff == '_'){
		//printf("match '_'\n");
	
		if(*(buff+1)== 'Z'){
			//printf("match 'Z'\n");
			buff += 2;

			if(*buff == 'Z'){
				buff++;
			}
			else if(!strncmp(buff, "ZN", 2)){
				buff += 2;
			}		
			else if(!strncmp(buff, "dl", 2)){
				AOP_COPY_TEST(new_buff, "operator delete", 2);
			}
			else if(!strncmp(buff, "da", 2)){
				AOP_COPY_TEST(new_buff, "operator delete[]", 2);
			}
			else if(!strncmp(buff, "na", 2)){
				AOP_COPY_TEST(new_buff, "operator new[]", 2);
			}			
			else if(!strncmp(buff, "eq", 2)){
				AOP_COPY_TEST(new_buff, "operator==", 2);
			}
			else if(!strncmp(buff, "lt", 2)){
				AOP_COPY_TEST(new_buff, "operator<", 2);
			}
			else if(!strncmp(buff, "nw", 2)){
				AOP_COPY_TEST(new_buff, "operator new", 2);
			}			
			else if(!strncmp(buff, "Thn", strlen("Thn"))){
				//printf("match 'Thn'\n");
				buff += strlen("Thn");
				while((*buff != '_' ) && (*buff != '\0'))
				{
					buff++;
				}
				AOP_COPY_TEST(new_buff, "non-virtual thunk to ", 1);
			}	
			else if(!strncmp(buff, "Tv0_n", strlen("Tv0_n"))){
				//printf("match 'Tv0_n'\n");
				buff += strlen("Tv0_n");
				while((*buff != '_' ) && (*buff != '\0'))
				{
					buff++;
				}
				AOP_COPY_TEST(new_buff, "virtual thunk to  ", 1);
			}
			else if(!strncmp(buff, "St", 2)){
				//printf("match 'St', cur_len = %d\n", cur_len);
				AOP_COPY_TEST(new_buff, "std::", 2);
			}
			else if(!strncmp(buff, "TI", 2)){
				AOP_COPY_TEST(new_buff, "typeinfo for ", 2);
			}
			else if(!strncmp(buff, "TS", 2)){
				AOP_COPY_TEST(new_buff, "typeinfo name for ", 2);
			}
			else if(!strncmp(buff, "TT", 2)){
				AOP_COPY_TEST(new_buff, "VTT for ", 2);
			}
			else if(!strncmp(buff, "TV", 2)){
				AOP_COPY_TEST(new_buff, "vtable for ", 2);
			}	
			else if(!strncmp(buff, "TC", 2)){
				AOP_COPY_TEST(new_buff, "construction vtable for ", 2);
			}	

			if(!strncmp(buff, "St", 2)){
				AOP_COPY_TEST(new_buff, "std::", 2);
			}
						
			if(*buff == 'N'){
				//printf("match 'N'\n");
				buff++;
				
				if(*buff == 'K'){
					//printf("match 'K'\n");
					buff++;
				}
				else if(!strncmp(buff, "Si", strlen("Si"))){
					//printf("match 'Si'\n");
					AOP_COPY_TEST(new_buff, "std::basic_istream<char, std::char_traits<char> >::", 2);
				}
				else if(!strncmp(buff, "SaIcE", 5)){
					AOP_COPY_TEST(new_buff, "std::allocator<char>", 5);
				}	
				else if(!strncmp(buff, "SaIwE", 5)){
					AOP_COPY_TEST(new_buff, "std::allocator<wchar_t>", 5);
				}
				else if(!strncmp(buff, "So", 2)){
					AOP_COPY_TEST(new_buff, "std::basic_ostream<char, std::char_traits<char> >", 2);
				}				
				
				if(!strncmp(buff, "SbIw", 4)){
					//printf("match 'SbIw'\n");
					AOP_COPY_TEST(new_buff, "std::basic_string<wchar_t, ", 4);
				}
				else if(!strncmp(buff, "Ss", 2)){
					AOP_COPY_TEST(new_buff, "std::basic_string<wchar, ", 2);
				}
				
				if(!strncmp(buff, "St", 2)){
					//printf("match 'St'\n");
					AOP_COPY_TEST(new_buff, "std::", 2);
				}
					
				len = aop_sym_demangled_ident_len(buff);
				if(len < 0){
					snprintf(&new_buff[cur_len], buf_len, buff);
					return 0;
				}
			
				if(len >= 10)
					buff += 2;
				else 
					buff += 1;
				//printf("buff = %s\n", buff);
				AOP_COPY_BUFF_TEST(new_buff, buff, len);

				if(*(buff+len) == 'C'){
					//printf("match 'C'\n");
					AOP_COPY_BUFF_TEST(new_buff, "::", 2);

					AOP_COPY_BUFF_TEST(new_buff, buff, len);
					new_buff[cur_len]  = '\0';
					return 0;
				}
				else if(*(buff+len) == 'D'){
					//printf("match 'D'\n");
					AOP_COPY_BUFF_TEST(new_buff, "::", 2);

					AOP_COPY_BUFF_TEST(new_buff, "~", 1);

					AOP_COPY_BUFF_TEST(new_buff, buff, len);
					new_buff[cur_len]  = '\0';
					return 0;
				}

				buff += len;

			}
			else
			{
			        len = aop_sym_demangled_ident_len(buff);
				if(len < 0){
					snprintf(&new_buff[cur_len], buf_len, buff);
					return 0;
				}
				if(len >= 10)
					buff += 2;
				else 
					buff += 1;	

				AOP_COPY_BUFF_TEST(new_buff, buff, len);
				new_buff[cur_len] = '\0';
				return 0;
			}
			
			while(*buff != '\0')
			{
				if(!strncmp(buff, "eqERKS0_", (temp_id_len = strlen("eqERKS0_") ))){
					AOP_COPY_TEST(new_buff, "::operator==", temp_id_len);
				}
				else if(!strncmp(buff, "neERKS", (temp_id_len = strlen("neERKS") ))){
					AOP_COPY_TEST(new_buff, "::operator!=", temp_id_len);
				}
				else if(!strncmp(buff, "ltERKS0_", (temp_id_len = strlen("ltERKS0_") ))){
					AOP_COPY_TEST(new_buff, "::operator<", temp_id_len);
				}
				else if(!strncmp(buff, "gtERKS0_", (temp_id_len = strlen("gtERKS0_") ))){
					AOP_COPY_TEST(new_buff, "::operator>", temp_id_len);
				}
				else if(!strncmp(buff, "aSE", (temp_id_len = strlen("aSE") ))){
					AOP_COPY_TEST(new_buff, "::operator=", temp_id_len);
				}				
				else if(!strncmp(buff, "geERKS_", (temp_id_len = strlen("geERKS_") ))){
					AOP_COPY_TEST(new_buff, "::operator>=", temp_id_len);
				}
				else if(!strncmp(buff, "leERKS_", (temp_id_len = strlen("leERKS_") ))){
					AOP_COPY_TEST(new_buff, "::operator<=", temp_id_len);
				}	
				else if(!strncmp(buff, "miERKS_", (temp_id_len = strlen("miERKS_") ))){
					AOP_COPY_TEST(new_buff, "::operator-", temp_id_len);
				}				
				else if(!strncmp(buff, "dVERKS_", (temp_id_len = strlen("dVERKS_") ))){
					AOP_COPY_TEST(new_buff, "::operator/=", temp_id_len);
				}
				else if(!strncmp(buff, "mLE", (temp_id_len = strlen("mLE") ))){
					AOP_COPY_TEST(new_buff, "::operator*=", 3);
				}				
				else if(!strncmp(buff, "pLE", (temp_id_len = strlen("pLE") ))){
					AOP_COPY_TEST(new_buff, "::operator+=", 3);
				}
				else if(!strncmp(buff, "rMERKS_", (temp_id_len = strlen("rMERKS_") ))){
					AOP_COPY_TEST(new_buff, "::operator%=", temp_id_len);
				}	
				else if(!strncmp(buff, "mIE", 3)){
					AOP_COPY_TEST(new_buff, "::operator-=", 3);
				}
				else if(!strncmp(buff, "dl", 2)){
					AOP_COPY_TEST(new_buff, "::operator delete", 2);
				}
				else if(!strncmp(buff, "eq", 2)){
					AOP_COPY_TEST(new_buff, "::operator==", 2);
				}
				else if(!strncmp(buff, "lt", 2)){
					AOP_COPY_TEST(new_buff, "::operator<", 2);
				}
				else if(!strncmp(buff, "nw", 2)){
					AOP_COPY_TEST(new_buff, "::operator new", 2);
				}
				else if(!strncmp(buff, "gtE", 3)){
					AOP_COPY_TEST(new_buff, "::operator>", 3);
				}
				else if(!strncmp(buff, "clER", 4)){
					AOP_COPY_TEST(new_buff, "::operator", 4);
				}
				else if(!strncmp(buff, "plE", 3)){
					AOP_COPY_TEST(new_buff, "::operator+", 3);
				}
				else if(!strncmp(buff, "ngE", 3)){
					AOP_COPY_TEST(new_buff, "::operator-", 3);
				}
			
				len = aop_sym_demangled_ident_len(buff);
				if(len <= 0)
					break;
				
				if(len >= 10)
					buff += 2;
				else 
					buff += 1;
				
				AOP_COPY_BUFF_TEST(new_buff, "::", 2);

				AOP_COPY_BUFF_TEST(new_buff, buff, len);

				if(*(buff+len) == 'C'){
					//printf("match 'C'\n");
					AOP_COPY_BUFF_TEST(new_buff, "::", 2);

					AOP_COPY_BUFF_TEST(new_buff, buff, len);
				}
				else if(*(buff+len) == 'D'){
					//printf("match 'D'\n");

					AOP_COPY_BUFF_TEST(new_buff, "::", 2);

					AOP_COPY_BUFF_TEST(new_buff, "~", 1);

					AOP_COPY_BUFF_TEST(new_buff, buff, len);
				}

				buff += len;
			}
			new_buff[cur_len]  = '\0';
			return 0;
		}
	}

	AOP_COPY_BUFF_TEST(new_buff, buff, (int)strlen(buff));	
	new_buff[cur_len] = '\0';
	return 0;
}


