#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

struct JSONObject
{
    JSONObject(const std::unordered_map<std::string, const JSONObject*>& properties);

    static JSONObject* deserialize(std::stringstream& stream);

    virtual void serialize(std::stringstream& stream) const;

    const JSONObject* getProperty(const std::string name) const;

    virtual std::optional<std::string> asString() const;

private:
    const std::unordered_map<std::string, const JSONObject*> properties;

};

struct JSONString : public JSONObject
{
    JSONString(const std::string str);

    static JSONString* deserialize(std::stringstream& stream);

    void serialize(std::stringstream& stream) const override;

    std::optional<std::string> asString() const override;

private:
    const std::string str;

};

struct Message
{
    Message(const JSONObject* data);

    static Message* deserialize(std::stringstream& stream);

    void serialize(std::stringstream& stream) const;

    const JSONObject* data;
};
