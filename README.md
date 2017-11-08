# ec2-pricing

```
# rapidJSON is required
g++ -std=c++11 -O2 -Wall -o ec2-pricing ec2-pricing.cpp

curl https://pricing.us-east-1.amazonaws.com/offers/v1.0/aws/index.json > /tmp/aws-ec2-pricing.json
./ec2-pricing /tmp/aws-ec2-pricing.json
```
