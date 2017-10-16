#include <iostream>
#include "../include/squll.h"

struct user {
  unsigned id;
  std::string name;
};

int main() {
  using namespace squll;
  auto schema = squll::schema("test.db",
			      table("users",
				    column("id", &user::id,
					   squll::constraints::autoincrement()),
				    column("name", &user::name)
				    )
			      
			      );
  return 0;
}
