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

int main() {
  using namespace squll;
  auto schema = squll::schema("test.db",
			      table("users",
				    column("id", &user::id,
					   constraints::autoincrement()),
				    column("name", &user::name,
					   constraints::not_null()
					   )
				    ),
			      table("books",
				    column("id", &book::id,
					   constraints::autoincrement()),
				    column("title", &book::name,
					   constraints::not_null())
				    )
			      
			      );
  return 0;
}
