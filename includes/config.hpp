#ifndef CONFIG_HPP
# define CONFIG_HPP
# include <string>
# include <vector>
# include <map>

struct LocationConfig {
    std::string              path;
    std::string              root;
    std::string              index;
    std::vector<std::string> methods;
    bool                     autoindex;
    std::string              upload_dir;
    size_t                   client_max_body_size;
    std::string              cgi_extension;
    std::string              cgi_path;

    LocationConfig()
        : autoindex(false), client_max_body_size(0) {}
};

struct ServerConfig {
    int                         port;
    std::string                 server_name;
    size_t                      client_max_body_size;
    std::map<int, std::string>  error_pages;
    std::vector<LocationConfig> locations;

    ServerConfig()
        : port(80), client_max_body_size(1048576) {} // default: 1MB
};

#endif
