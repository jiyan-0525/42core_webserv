NAME		= webserv

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++17
INCLUDES	= -I includes

SRCS_DIR	= srcs
OBJS_DIR	= objs

SRCS		=  main.cpp \
			  $(SRCS_DIR)/config/configParser.cpp \
			  $(SRCS_DIR)/http/httpRequest.cpp \
			  $(SRCS_DIR)/http/httpResponse.cpp \
			  $(SRCS_DIR)/server/server.cpp\
			  $(SRCS_DIR)/server/signals/signals.cpp\
			  $(SRCS_DIR)/handlers/requestHandler.cpp\
			  $(SRCS_DIR)/handlers/handleCGI.cpp\
			  $(SRCS_DIR)/handlers/requesHanddlerUtils.cpp\

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
