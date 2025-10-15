#include "ConfigParser.h"
#include "../utils/Logger.h"

#include <filesystem>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

const std::string ConfigParser::CONFIG_FILE_NAME = "config.conf";
const char ConfigParser::COMMENT_CHAR = '*';
const char ConfigParser::CONFIG_KEY_SEPARATOR = '=';

ConfigParser::ConfigParser()
{

    std::string configPath = getConfigFilePath();
    createConfigFileIfNotExists(configPath);

    m_config_file.open(configPath);
    if (!m_config_file.is_open())
    {
        throw std::runtime_error("Cound't find the configuration file");
    }
}

void ConfigParser::read()
{
    std::string line;
    while (std::getline(m_config_file, line))
    {
        if (!line.empty() && line[0] != COMMENT_CHAR)
        {
            size_t separator_index = line.find(CONFIG_KEY_SEPARATOR);
            std::string config_key = line.substr(0, separator_index);
            std::string config_value = line.substr(separator_index + 1, line.size() - 1);
            m_config[config_key] = config_value;
        }
    }
}

std::string ConfigParser::getConfigFilePath() const
{
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path)))
    {
        return std::string(path) + "\\PulseNet\\" + CONFIG_FILE_NAME;
    }
#else
    return std::string(getenv("HOME")) + "/.config/PulseNet/" + CONFIG_FILE_NAME;
#endif
}

void ConfigParser::createConfigFileIfNotExists(const std::string &configPath)
{
    std::filesystem::path parentPath = configPath;
    std::filesystem::create_directories(parentPath.parent_path());

    std::ifstream configFile(configPath);
    Logger &logger = Logger::getInstance();

    if (!configFile)
    {
        std::ofstream newConfigFile(configPath);
        if (newConfigFile)
        {
            newConfigFile << "\n";
            newConfigFile << "*******************************************************"
                             "****************\n";
            newConfigFile << "*                           DATABASE CONFIG            "
                             "               *\n";
            newConfigFile << "*******************************************************"
                             "****************\n ";
            newConfigFile << "\n";
            newConfigFile << "DATABASE_HOST=localhost\n";
            newConfigFile << "DATABASE_PORT=5432\n";
            newConfigFile << "DATABASE_PASSWORD=root\n";
            newConfigFile << "\n";
            newConfigFile << "\n";
            newConfigFile.close();
            logger.log(LogType::APPLICATION, LogSeverity::LOG_INFO, "Created config file at: " + configPath);
        }
        else
        {
            throw std::runtime_error("Error creating config file at: " + configPath);
        }
    }
    else
    {
        logger.log(LogType::APPLICATION, LogSeverity::LOG_INFO, "Config file already exists at: " + configPath);
    }
}

std::ostream &operator<<(std::ostream &os, const ConfigParser &cp)
{
    std::map<std::string, std::string>::const_iterator it;
    for (it = cp.m_config.begin(); it != cp.m_config.end(); it++)
    {
        os << it->first << "=" << it->second << '\n';
    }

    return os;
}
