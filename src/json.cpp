#include "../include/json.h"

JSONObject::JSONObject(const std::unordered_map<std::string, const JSONObject*>& properties) :
    properties(properties) {}

JSONObject* JSONObject::deserialize(std::stringstream& stream)
{
    stream.get();

    std::unordered_map<std::string, const JSONObject*> properties;

    while (stream.peek() != '}' && !stream.eof())
    {
        std::string name;

        while (stream.peek() != ':' && !stream.eof())
        {
            name += stream.get();
        }

        if (stream.eof())
        {
            return nullptr;
        }

        if (properties.count(name))
        {
            return nullptr;
        }

        stream.get();

        if (stream.peek() == '"')
        {
            const JSONObject* str = JSONString::deserialize(stream);

            if (!str)
            {
                return nullptr;
            }

            properties[name] = str;
        }

        else if (stream.peek() == '{')
        {
            const JSONObject* obj = JSONObject::deserialize(stream);

            if (!obj)
            {
                return nullptr;
            }

            properties[name] = obj;
        }

        else
        {
            return nullptr;
        }

        if (stream.peek() == ',')
        {
            stream.get();
        }

        else if (stream.peek() != '}')
        {
            return nullptr;
        }
    }

    if (stream.eof())
    {
        return nullptr;
    }

    stream.get();

    return new JSONObject(properties);
}

void JSONObject::serialize(std::stringstream& stream) const
{
    stream << '{';

    bool first = true;

    for (const std::pair<std::string, const JSONObject*> property : properties)
    {
        if (first)
        {
            first = false;
        }

        else
        {
            stream << ',';
        }

        stream << property.first << ':';

        property.second->serialize(stream);
    }

    stream << '}';
}

const JSONObject* JSONObject::getProperty(const std::string name) const
{
    if (properties.count(name))
    {
        return properties.at(name);
    }

    return new JSONObject({});
}

std::optional<std::string> JSONObject::asString() const
{
    return std::nullopt;
}

JSONString::JSONString(const std::string str) :
    JSONObject({}), str(str) {}

JSONString* JSONString::deserialize(std::stringstream& stream)
{
    stream.get();

    std::string str;

    while (stream.peek() != '"' && !stream.eof())
    {
        str += stream.get();
    }

    if (stream.eof())
    {
        return nullptr;
    }

    stream.get();

    return new JSONString(str);
}

void JSONString::serialize(std::stringstream& stream) const
{
    stream << '"' << str << '"';
}

std::optional<std::string> JSONString::asString() const
{
    return str;
}

Message::Message(const JSONObject* data) :
    data(data) {}

Message* Message::deserialize(std::stringstream& stream)
{
    char buffer[9];

    stream.get(buffer, 9);

    if (strncmp(buffer, "squirrel", 8) != 0)
    {
        return nullptr;
    }

    if (stream.peek() != '{')
    {
        return nullptr;
    }

    return new Message(JSONObject::deserialize(stream));
}

void Message::serialize(std::stringstream& stream) const
{
    stream << 's' << 'q' << 'u' << 'i' << 'r' << 'r' << 'e' << 'l';

    data->serialize(stream);
}
