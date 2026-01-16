#include "../include/flags.h"

Flags* Flags::parse(const int argc, char** argv, ErrorHandler* errorHandler)
{
    Flags* flags = new Flags();

    for (int i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], "--service", 9) == 0)
        {
            if (flags->type == LaunchType::Service)
            {
                errorHandler->handle(SquirrelArgumentException("Argument \"--service\" specified more than once."));

                return nullptr;
            }

            if (flags->type == LaunchType::Receive)
            {
                errorHandler->handle(SquirrelArgumentException("Argument \"--service\" is not compatible with argument \"--receive\"."));

                return nullptr;
            }

            flags->type = LaunchType::Service;
        }

        else if (strncmp(argv[i], "--receive", 9) == 0)
        {
            if (flags->type == LaunchType::Service)
            {
                errorHandler->handle(SquirrelArgumentException("Argument \"--receive\" is not compatible with argument \"--service\"."));

                return nullptr;
            }

            if (flags->type == LaunchType::Receive)
            {
                errorHandler->handle(SquirrelArgumentException("Argument \"--receive\" specified more than once."));

                return nullptr;
            }

            flags->type = LaunchType::Receive;
        }

        else if (strncmp(argv[i], "--", 2) == 0)
        {
            errorHandler->handle(SquirrelArgumentException("Unknown argument \"" + std::string(argv[i]) + "\"."));

            return nullptr;
        }

        else
        {
            if (flags->type == LaunchType::Service)
            {
                errorHandler->handle(SquirrelArgumentException("Launch type \"--service\" does not expect any unnamed arguments."));

                return nullptr;
            }

            if (flags->type == LaunchType::Receive)
            {
                flags->ip = argv[i];
            }

            else
            {
                if (flags->path == "")
                {
                    flags->path = argv[i];

                    if (!std::filesystem::exists(flags->path))
                    {
                        errorHandler->handle(SquirrelArgumentException("Specified file \"" + flags->path + "\" does not exist."));

                        return nullptr;
                    }
                }

                else
                {
                    errorHandler->handle(SquirrelArgumentException("Too many file arguments specified."));

                    return nullptr;
                }
            }
        }
    }

    return flags;
}
