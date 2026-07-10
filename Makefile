NAME		= webserv

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98
INCLUDES	= -I includes

SRCS_DIR	= srcs
OBJS_DIR	= objs

SRCS		= $(SRCS_DIR)/main.cpp \
			  $(SRCS_DIR)/config/Config.cpp \
			  $(SRCS_DIR)/config/ConfigParser.cpp

OBJS		= $(SRCS:$(SRCS_DIR)/%.cpp=$(OBJS_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
