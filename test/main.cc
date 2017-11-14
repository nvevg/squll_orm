#include <iostream>
#include "../include/squll.h"

struct user {
  unsigned id;
  std::string name;
};

struct book {
  unsigned id;
  std::string name;
};

void test() {
  using namespace squll;
  
  auto schema = squll::schema("test.db",
  			      table("users",
  				    column("id", &user::id, constraints::not_null(), constraints::primary_key(true))
  				    )
			      );
  
}

int main() {

  test();
  
  return 0;
}
