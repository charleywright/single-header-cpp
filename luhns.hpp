#pragma once

#include <string>
#include <vector>
#include <iostream>

namespace luhns
{
    enum class provider
    {
        invalid = 0,
        visa,
        amex,
        master_card
    };

    inline const char *provider_str(provider p)
    {
      switch (p)
      {
        case provider::invalid:
          return "invalid";
        case provider::visa:
          return "visa";
        case provider::amex:
          return "american express";
        case provider::master_card:
          return "mastercard";
        default:
          return "unknown";
      }
    }

    class card_number
    {
    public:
        explicit card_number(std::string number) : number(std::move(number))
        {
          if (this->number.find('4') == 0)
          {
            this->provider = luhns::provider::visa;
          } else if ((this->number.find('3') == 0))
          {
            this->provider = luhns::provider::amex;
          } else if (
                  (this->number.find('2') == 0) ||
                  (this->number.find('5') == 0))
          {
            this->provider = luhns::provider::master_card;
          }

          std::size_t checksum = 0;
          for (std::size_t i = 0; i < this->number.size(); i++)
          {
            if (this->number.size() % 2 == i % 2)
            {
              checksum += card_number::add_digits(static_cast<int>(this->number[i] - '0') * 2);
            } else
            {
              checksum += static_cast<int>(this->number[i] - '0');
            }
          }
          this->valid = checksum % 10 == 0;
        }

        card_number(const card_number &copy) = default;
        card_number &operator=(const card_number &copy) = default;
        card_number(card_number &&move) = delete;
        card_number &operator=(card_number &&move) = delete;

        luhns::provider get_provider() const
        {
          return this->provider;
        }

        bool is_valid() const
        {
          return this->valid;
        }

    private:
        static int add_digits(int num)
        {
          int ret = 0;
          do
          {
            ret += num % 10;
            num /= 10;
          } while (num > 0);
          return ret;
        }

        std::string number;
        luhns::provider provider = luhns::provider::invalid;
        bool valid = false;
    };
}
