# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: bstorck <marvin@42.fr>                     +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/03/23 23:48:24 by bstorck           #+#    #+#              #
#    Updated: 2026/03/23 23:48:25 by bstorck          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME		=	webserv

CXX			=	c++
CXXFLAGS	=	-Werror -Wextra -Wall -std=c++98

SRCS_DIR	=	./srcs/
OBJS_DIR	=	./objs/
INCS_DIR	=	./incs/

SRCS		=	main.cpp Logger.cpp Parser.cpp Server.cpp Client.cpp \
			HTTPRequest.cpp HTTPResponse.cpp Dispatcher.cpp utils.cpp

OBJS		=	$(addprefix $(OBJS_DIR), $(SRCS:.cpp=.o))

INCS		=	-I $(INCS_DIR)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(INCS)

$(OBJS_DIR)%.o: $(SRCS_DIR)%.cpp | $(OBJS_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCS)

$(OBJS_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(OBJS_DIR)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all re clean fclean
