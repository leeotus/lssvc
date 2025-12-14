#include <iostream>
#include <json/json.h>

int main(int argc, char **argv) {
  Json::Value person;
  Json::Value root;
  person["name"] = "leeotus";
  person["age"] = 24;
  person["is_student"] = true;
  person["score"] = 99.9;
  root["person_info"] = person;

  Json::StreamWriterBuilder writerBuilder;
  writerBuilder["indentation"] = "  "; // indent
  std::string jsonStr = Json::writeString(writerBuilder, root);
  std::cout << "json:\n" << jsonStr << "\n\n";
  return 0;
}
