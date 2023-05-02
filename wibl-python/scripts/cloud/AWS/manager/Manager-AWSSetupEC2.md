# WIBL Cloud Manager Setup for AWS

DEPRECATED: This exists for reference purposes only. Please use the setup shell scripts in `scripts/cloud/AWS`.

This guide describes how to setup the WIBL Cloud Manager service on AWS using ECS with EC2. 
AWS CLI is used to perform most configuration steps. Some pre-requisite steps may make use
of the AWS console or other tools.

> Note: this is for reference purposes only as the need to have AWS PrivateLink connections to ECS and ECR
> (or a public IP address for container EC2 instances) made EC2 more expensive than Fargate, while also being more 
> complicated.

## [TEMPORARY] References

https://github.com/michaelact/example-ecs-deployment/tree/main/deploy/aws-cli
https://github.com/aws-samples/aws-containers-task-definitions/blob/master/gunicorn/gunicorn_ec2.json
https://docs.aws.amazon.com/cli/latest/userguide/cli-services-ec2-instances.html
https://peterwadid1.medium.com/start-stop-ecs-fargate-tasks-using-lambda-cloudwatch-events-rules-b453aad4e1f6
https://docs.aws.amazon.com/vpc/latest/userguide/vpc-subnets-commands-example-ipv6.html
https://docs.aws.amazon.com/AmazonECS/latest/developerguide/ECS_AWSCLI_EC2.html
https://docs.aws.amazon.com/AmazonECS/latest/developerguide/instance_IAM_role.html


## Getting started
Create an output directory in the same directory as this instruction markdown file:
```
mkdir output
```

## Creating an EC2 instance for running containers

### Prerequisites
setup IAM policies for EC2:
https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/iam-policies-for-amazon-ec2.html

### Setup `ecsInstanceRole`
First, follow the instructions for 
[Creating the container instance (`escInstanceRole`) role](https://docs.amazonaws.cn/en_us/AmazonECS/latest/developerguide/instance_IAM_role.html) 
from the AWS console.

Then, create an instance profile and add the `ecsInstanceRole` to it:
```shell
$ aws iam create-instance-profile --instance-profile-name ecsInstanceRole-profile | tee output/iam_create_instance_profile.json
$ aws iam add-role-to-instance-profile \
    --instance-profile-name ecsInstanceRole-profile \
    --role-name ecsInstanceRole
```

### Create SSH key pair
Create the keypair, saving the private key locally:
```shell
$ aws ec2 create-key-pair --key-name wibl-manager-key --query 'KeyMaterial' --output text > output/wibl-manager-private-key.pem
$ chmod 400 output/wibl-manager-private-key.pem
```

> To log into your EC2 instance(s), you will need to add the key to your SSH key store (e.g., copy to ~/.ssh)

Verify:
```shell
$ aws ec2 describe-key-pairs --key-name wibl-manager-key
```

### Create ECS cluster
Create ECS cluster to run services and tasks on our internal EC2 instance:
```shell
$ aws ecs create-cluster --cluster-name wibl-manager-cluster | tee output/ecs_create_cluster.json
```

### Create VPC and subnets
Reference documentation: https://docs.aws.amazon.com/vpc/latest/userguide/vpc-subnets-commands-example.html

Create a VPC with a 10.0.0.0/16:
```shell
$ aws ec2 create-vpc --cidr-block 10.0.0.0/16 --query Vpc.VpcId --output text | tee output/create_vpc.txt
```

> Delete with `aws ec2 delete-vpc --vpc-id $(cat output/create_vpc.txt)`

Create a subnet with a 10.0.1.0/24 CIDR block, this will be the public subnet:
```shell
$ aws ec2 create-subnet --vpc-id $(cat output/create_vpc.txt) --cidr-block 10.0.1.0/24 \
	--query Subnet.SubnetId --output text | tee output/create_subnet_public.txt 
```

> Delete with `aws ec2 delete-subnet --subnet-id $(cat output/create_subnet_public.txt)`

Create a subnet with a 10.0.2.0/24 CIDR block, this will be the private subnet:
```shell
$ aws ec2 create-subnet --vpc-id $(cat output/create_vpc.txt) --cidr-block 10.0.2.0/24 \
	--query Subnet.SubnetId --output text | tee output/create_subnet_private.txt 
```

> Delete with `aws ec2 delete-subnet --subnet-id $(cat output/create_subnet_private.txt)`

#### Configure public subnet

Create Internet gateway:
```shell
$ aws ec2 create-internet-gateway --query InternetGateway.InternetGatewayId \
  --output text | tee output/create_internet_gateway.txt
```

Attach the Internet gateway to the VPC:
```shell
$ aws ec2 attach-internet-gateway --vpc-id $(cat output/create_vpc.txt) \
  --internet-gateway-id $(cat output/create_internet_gateway.txt)
```

Detach with:
```shell
aws ec2 detach-internet-gateway --vpc-id $(cat output/create_vpc.txt) \
  --internet-gateway-id $(cat output/create_internet_gateway.txt)
```

Create a custom routing table for the VPC:
```shell
$ aws ec2 create-route-table --vpc-id $(cat output/create_vpc.txt) \
  --query RouteTable.RouteTableId --output text | tee output/create_route_table_ingress.txt
```

> Delete with `aws ec2 delete-route-table --route-table-id $(cat output/create_route_table_ingress.txt)` 

Create route pointing traffic to the Internet gateway:
```shell
$ aws ec2 create-route --route-table-id $(cat output/create_route_table_ingress.txt) \
  --destination-cidr-block 0.0.0.0/0 \
  --gateway-id $(cat output/create_internet_gateway.txt)
```

Confirm that the routing table has been setup as expected:
```shell
$ aws ec2 describe-route-tables --route-table-id $(cat output/create_route_table_ingress.txt)
```

Associate the custom routing table with the public subnet:
```shell
$ aws ec2 associate-route-table --subnet-id $(cat output/create_subnet_public.txt) \
  --route-table-id $(cat output/create_route_table_ingress.txt)
```

Enable public IP assignment when instances are launched in this subnet:
```shell
$ aws ec2 modify-subnet-attribute --subnet-id $(cat output/create_subnet_public.txt) \
  --map-public-ip-on-launch
```

#### Configure private subnet
[I think we can skip this if we don't want outbound connections]

Configure egress-only gateway for private subnet:
```shell
$ aws ec2 create-egress-only-internet-gateway --vpc-id $(cat output/create_vpc.txt) \
  --output text | tee output/create_egress_gateway.txt
```

Create a route table for the private subnet:
```shell
$ aws ec2 create-route-table --vpc-id $(cat output/create_vpc.txt) \
  --query RouteTable.RouteTableId --output text | tee output/create_route_table_egress.txt
```

Create route to egress-only gateway:
> Note: this is broken as IPv4 routes aren't allowed for egress-only gateways, only IPv6.
> Use a NAT gateway for IPv4 traffic instead
```shell
$ aws ec2 create-route --route-table-id $(cat output/create_route_table_egress.txt) \
  --destination-cidr-block 0.0.0.0/0 \
  --egress-only-internet-gateway-id $(head -n 1 output/create_egress_gateway.txt | awk '{print $2}')
```

Associate the egress routing table with the private subnet:
```shell
$ aws ec2 associate-route-table --subnet-id $(cat output/create_subnet_private.txt) \
  --route-table-id $(output/create_route_table_egress.txt)
```

Enable public IP assignment when instances are launched in this subnet:
```shell
$ aws ec2 modify-subnet-attribute --subnet-id $(cat output/create_subnet_private.txt) \
  --map-public-ip-on-launch
```

### Launch instances 

#### Launch test instance into public subnet
The test instance will allow us to debug the WIBL manager service running in ECS.

Create security group
```shell
$ aws ec2 create-security-group \
	--group-name wibl-manager-ssh-inet \
	--vpc-id $(cat output/create_vpc.txt) \
	--description "SSH from Internet" --output text | tee output/create_security_group_ssh_inet.txt
```

> Delete with `aws ec2 delete-security-group --group-id $(cat output/create_security_group_ssh_inet.txt)`

Authorize ingress from ccom.unh.edu public IP:
```shell
$ aws ec2 authorize-security-group-ingress --group-id $(cat output/create_security_group_ssh_inet.txt) \
  --ip-permissions '[{"IpProtocol": "tcp", "FromPort": 22, "ToPort": 22, "IpRanges": [{"CidrIp": "132.177.103.226/32"}]}]'
```

Launch test EC2 instance with key, security group, and subnet created above:
```shell
$ aws ec2 run-instances --image-id ami-09f3bb43b9f607008 --count 1 --instance-type t4g.nano \
	--key-name wibl-manager-key --security-group-ids $(cat output/create_security_group_ssh_inet.txt) \
	--subnet-id $(cat output/create_subnet_public.txt) | tee output/run-test-ec2-instance.json
```

Tag the instance with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/run-test-ec2-instance.json | jq -r '.Instances[0].InstanceId') \
  --tags 'Key=Name,Value=wibl-manager-ec2-test'
```

> Note: This EC2 instance will be accessible from ccom.unh.edu via SSH and will also be able to access the private
> EC2 instance that will run ECS containers, and only has access to the private network.

Verify instance is running and get its IP address:
```shell
$ aws ec2 describe-instances
```

Try connecting to test instance via SSH (from ccom.unh.edu):
```shell
$ scp -i output/wibl-manager-private-key.pem USERNAME@ccom.unh.edu:
$ ssh USERNAME@ccom.unh.edu
$ ssh -i wibl-manager-private-key.pem ec2-user@EC2_PUBLIC_IP_ADDRESS
```

Then disconnect from the test instance and copy the SSH key there so that we can connect from the test instance
to the internal instance that will run our ECS containers:
```shell
scp wibl-manager-private-key.pem ec2-user@EC2_PUBLIC_IP_ADDRESS:
```

Stop test EC2 instance:
```shell
$ aws ec2 stop-instances --instance-ids $(cat output/run-test-ec2-instance.json | jq -r '.Instances[0].InstanceId')
```

Start test EC2 instance:
```shell
$ aws ec2 start-instances --instance-ids $(cat output/run-test-ec2-instance.json | jq -r '.Instances[0].InstanceId')
```

Delete test instance:
```shell
$ aws ec2 terminate-instances --instance-ids $(cat output/run-test-ec2-instance.json | jq -r '.Instances[0].InstanceId')
```

#### Launch into private subnet (where our ECS containers will be run)

Create security group
```shell
$ aws ec2 create-security-group \
	--group-name wibl-manager-private \
	--vpc-id $(cat output/create_vpc.txt) \
	--description "Security Group for AWS ECS EC2 instances" \
	--output text | tee output/create_security_group_private.txt
```

> Delete with `aws ec2 delete-security-group --group-id $(cat output/create_security_group_private.txt)`


Configure private security group to allow SSH, HTTP, HTTPS, and ICMP connections from the private subnet:
```shell
$ aws ec2 authorize-security-group-ingress --group-id $(cat output/create_security_group_private.txt) \
  --ip-permissions '[{"IpProtocol": "tcp", "FromPort": 22, "ToPort": 22, "IpRanges": [{"CidrIp": "10.0.0.0/16"}]}, {"IpProtocol": "tcp", "FromPort": 80, "ToPort": 80, "IpRanges": [{"CidrIp": "10.0.0.0/16"}]}, {"IpProtocol": "tcp", "FromPort": 443, "ToPort": 443, "IpRanges": [{"CidrIp": "10.0.0.0/16"}]}]'
$ aws ec2 authorize-security-group-ingress --group-id $(cat output/create_security_group_private.txt) \
  --ip-permissions '[{"IpProtocol": "imcp", "FromPort": -1, "ToPort": -1, "IpRanges": [{"CidrIp": "10.0.0.0/16"}]}]'
```

Launch EC2 instance for running ECS containers with key, security group, and subnet created above:
```shell
$ aws ec2 run-instances --image-id ami-056fe1bf7b255fb89 --count 1 --instance-type t4g.nano \
    --iam-instance-profile 'Name=ecsInstanceRole-profile' \
	--key-name wibl-manager-key --security-group-ids $(cat output/create_security_group_private.txt) \
	--subnet-id $(cat output/create_subnet_private.txt) \
	--user-data file://input/set_ecs_cluster.txt | tee output/run-container-ec2-instance.json
```

Tag the instance with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/run-container-ec2-instance.json | jq -r '.Instances[0].InstanceId') \
  --tags 'Key=Name,Value=wibl-manager-ec2-containers'
```

Verify instance is running and get its IP address:
```shell
$ aws ec2 describe-instances
```

Try connecting to instance via SSH (from test instance):
```shell
$ ssh -i wibl-manager-private-key.pem ec2-user@IPADDRESS
```

Stop ECS container EC2 instance:
```shell
$ aws ec2 stop-instances \
  --instance-ids $(cat output/run-container-ec2-instance.json | jq -r '.Instances[0].InstanceId')
```

Start ECS container EC2 instance:
```shell
$ aws ec2 start-instances --instance-ids $(cat output/run-container-ec2-instance.json | jq -r '.Instances[0].InstanceId')
```

Delete ECS container EC2 instance:
```shell
$ aws ec2 terminate-instances --instance-ids $(cat output/run-container-ec2-instance.json | jq -r '.Instances[0].InstanceId')
```

## Setup ECR container registry
Create repo:
```shell
$ aws ecr create-repository \
  --repository-name wibl/manager | tee output/create_ecr_repository.json
```

`docker login` to the repo so that we can push to it:
```shell
$ aws ecr get-login-password | docker login \
  --username AWS \
  --password-stdin \
  $(cat output/create_ecr_repository.json | jq -r '.repository.repositoryUri')
```

If `docker login` is successful, you should see `Login Succeeded` printed to STDOUT.
