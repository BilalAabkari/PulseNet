#pragma once
#include <fstream>
#include <iostream>
#include <map>
#include <string>

/**
 * @class ConfigParser
 * @brief A class that parses config.
 *
 * This class reads and parses the configuration file storing all the params.
 * Provides methods for retrieveng params related to different parts of the
 * application like database credentials, cache parameters...
 */
class ConfigParser
{
  public:
    /* ----------------
     * public structs
     * ----------------
     */
    struct ssl_config
    {
        std::string ssl_certificate;
        std::string ssl_certificate_key;
    };

    /* ----------------
     * Constructors
     * ----------------
     */
    ConfigParser();

    /* ----------------
     * Public methods
     * ----------------
     */

    void read();

    std::string getConfigFilePath() const;

    ssl_config getSSLConfig() const;

    /* ----------------
     * Overloaded operators
     * ----------------
     */
    friend std::ostream &operator<<(std::ostream &os, const ConfigParser &cp);

  private:
    /* ----------------
     * Private constants
     * ----------------
     */
    static const std::string CONFIG_FILE_NAME;
    static const char COMMENT_CHAR;
    static const char CONFIG_KEY_SEPARATOR;

    /* ----------------
     * Private attributes
     * ----------------
     */
    std::ifstream m_config_file;
    std::map<std::string, std::string> m_config;

    /* ----------------
     * Private methods
     * ----------------
     */

    void createConfigFileIfNotExists(const std::string &configPath);
};
