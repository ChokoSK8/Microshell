NAME	= microshell

SRCS	= microshell.c

OBJS	= $(SRCS:.c=.o)

FLAGS	= -Wall -Werror -Wextra

INC		= -I includes/

RM		= rm -rf

%.o: %.c
		gcc $(FLAGS) $(INC) -o $@ -c $?

all:	$(NAME)

$(NAME):	$(OBJS)
		gcc $(FLAGS) $(INC) $(OBJS) -o $(NAME)

clean:
		$(RM) $(OBJS)

fclean:		clean
			$(RM) $(NAME)

re:			fclean all

.PHONY:		all clean fclean re
