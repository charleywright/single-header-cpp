#include "./luhns.hpp"
#include <vector>
#include <iostream>

int main(int argc, char **argv)
{
  if (argc > 1)
  {
    luhns::card_number number(argv[1]);
    std::cout << "Type: " << luhns::provider_str(number.get_provider()) << " "
              << "Valid: " << std::boolalpha << number.is_valid() << std::noboolalpha << std::endl;
    return 0;
  }

  /*
    https://developer.paypal.com/api/nvp-soap/payflow/integration-guide/test-transactions/#standard-test-cards
    American Express    378282246310005
    American Express    371449635398431
    Mastercard          2221000000000009
    Mastercard          2223000048400011
    Mastercard          2223016768739313
    Mastercard          5555555555554444
    Mastercard          5105105105105100
    Visa                4111111111111111
    Visa                4012888888881881
  */

  const std::vector<std::string> amex = {
      "378282246310005",
      "371449635398431"};
  const std::vector<std::string> mastercard = {
      "2221000000000009",
      "2223000048400011",
      "2223016768739313",
      "5555555555554444",
      "5105105105105100"};
  const std::vector<std::string> visa = {
      "4111111111111111",
      "4012888888881881"};

  std::cout << "American Express:\n"
            << std::boolalpha;
  for (const auto &n : amex)
  {
    luhns::card_number number(n);
    std::cout << "Type: " << luhns::provider_str(number.get_provider()) << " Valid: " << number.is_valid() << std::endl;
  }

  std::cout << "Mastercard:\n";
  for (const auto &n : mastercard)
  {
    luhns::card_number number(n);
    std::cout << "Type: " << luhns::provider_str(number.get_provider()) << " Valid: " << number.is_valid() << std::endl;
  }

  std::cout << "VISA:\n";
  for (const auto &n : visa)
  {
    luhns::card_number number(n);
    std::cout << "Type: " << luhns::provider_str(number.get_provider()) << " Valid: " << number.is_valid() << std::endl;
  }
}
