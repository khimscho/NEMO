# WIBL Cloud Manager Setup for AWS

DEPRECATED: This exists for reference purposes only. Please use the setup shell scripts in `scripts/cloud/AWS`.

This guide describes how to setup the WIBL Cloud Manager service on AWS using ECS with Fargate
as a capacity provider (rather than EC2). 

AWS CLI is used to perform most configuration steps. Some pre-requisite steps may make use
of the AWS console or other tools.

## Getting started
Install dependencies:
 - [jq](https://stedolan.github.io/jq/)

Create an output directory in the same directory as this instruction markdown file:
```shell
$ mkdir output
```

Set region and account ID from AWS configuration so that subsequent commands can easily use it:
```shell
$ export AWS_REGION=$(aws configure get region)
$ export AWS_ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)
```

## Setup ECR container registry
Create repo:
```shell
$ aws --region $AWS_REGION ecr create-repository \
  --repository-name wibl/manager | tee output/create_ecr_repository.json
```

`docker login` to the repo so that we can push to it:
```shell
$ aws --region $AWS_REGION ecr get-login-password | docker login \
  --username AWS \
  --password-stdin \
  $(cat output/create_ecr_repository.json | jq -r '.repository.repositoryUri')
```

If `docker login` is successful, you should see `Login Succeeded` printed to STDOUT.

Build image and push to ECR repo
```shell
$ docker build -t wibl/manager ../../../../wibl-manager/
$ docker tag wibl/manager:latest ${AWS_ACCOUNT_ID}.dkr.ecr.${AWS_REGION}.amazonaws.com/wibl/manager:latest
$ docker push ${AWS_ACCOUNT_ID}.dkr.ecr.${AWS_REGION}.amazonaws.com/wibl/manager:latest | tee output/docker_push_to_ecr.txt
```

## Create ECS cluster
```shell
$ aws --region $AWS_REGION ecs create-cluster \
	--cluster-name wibl-manager-ecs \
	--capacity-providers FARGATE | tee output/create_ecs_cluster.json
```

## Setup and attach `ecsInstanceRole`
Create `ecsInstanceRole`:
```shell
$ aws --region $AWS_REGION iam create-role \
	--role-name ecsTaskExecutionRole \
	--assume-role-policy-document file://input/task-execution-assume-role.json | tee output/create_task_exec_role.json
```

If the role already exists, you will get an error to the following:
```
An error occurred (EntityAlreadyExists) when calling the CreateRole operation: Role with name ecsTaskExecutionRole already exists.
```

This is okay and can be ignored.

Next, attach role policy:
```shell
$ aws --region $AWS_REGION iam attach-role-policy \
	--role-name ecsTaskExecutionRole \
	--policy-arn arn:aws:iam::aws:policy/service-role/AmazonECSTaskExecutionRolePolicy | tee output/attach_role_policy.json
```

## Create VPC and subnet
Create a VPC with a 10.0.0.0/16 address block:
```shell
$ aws --region $AWS_REGION ec2 create-vpc --cidr-block 10.0.0.0/16 --query Vpc.VpcId --output text | tee output/create_vpc.txt
```

Enable DNS hostnames, which is required for connecting to EFS volumes:
```shell
$ aws --region $AWS_REGION ec2 modify-vpc-attribute --vpc-id $(cat output/create_vpc.txt) \
  --enable-dns-hostnames
```

> Delete with `aws ec2 delete-vpc --vpc-id $(cat output/create_vpc.txt)`

Create a subnet with a 10.0.0.0/24 CIDR block, this will be the private subnet:
```shell
$ aws --region $AWS_REGION ec2 create-subnet --vpc-id $(cat output/create_vpc.txt) --cidr-block 10.0.0.0/24 \
	--query Subnet.SubnetId --output text | tee output/create_subnet_private.txt 
```

> Delete with `aws ec2 delete-subnet --subnet-id $(cat output/create_subnet_private.txt)`

Tag the subnet with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/create_subnet_private.txt ) \
  --tags 'Key=Name,Value=wibl-private-subnet'
```

Create a routing table for private subnet to the VPC:
```shell
$ aws --region $AWS_REGION ec2 create-route-table --vpc-id $(cat output/create_vpc.txt) \
  --query RouteTable.RouteTableId --output text | tee output/create_route_table_private.txt
```

Tag the route table with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/create_route_table_private.txt) \
  --tags 'Key=Name,Value=wibl-private-rt'
```

Associate the custom routing table with the private subnet:
```shell
$ aws --region $AWS_REGION ec2 associate-route-table --subnet-id $(cat output/create_subnet_private.txt) \
  --route-table-id $(cat output/create_route_table_private.txt)
```

## Create security group and rule
Create security group to give us control over ingress/egress:
```shell
$ aws --region $AWS_REGION ec2 create-security-group \
	--group-name wibl-mgr-ecs-fargate \
	--vpc-id $(cat output/create_vpc.txt) \
	--description "Security Group for WIBL lambdas and WIBL Manager on ECS Fargate" \
	| tee output/create_security_group_private.json
```

> Delete with `aws ec2 delete-security-group --group-id $(cat output/create_security_group_private.json | jq -r '.GroupId')`

Tag the security group with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  --tags 'Key=Name,Value=wibl-mgr-ecs-fargate'
```

Create ingress rule to the load balancer to access the manager via port 8000
```shell
$ aws --region $AWS_REGION ec2 authorize-security-group-ingress \
  --group-id $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  --ip-permissions '[{"IpProtocol": "tcp", "FromPort": 8000, "ToPort": 8000, "IpRanges": [{"CidrIp": "10.0.0.0/24"}]}]' \
  | tee output/create_security_group_private_rule_lambda.json
```

Tag the load balancer ingress rule with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/create_security_group_private_rule_lambda.json | jq -r '.SecurityGroupRules[0].SecurityGroupRuleId') \
  --tags 'Key=Name,Value=wibl-manager-elb'
```

OLD Create temporary HTTP(S) ingress rule to allow debugging (eventually we will probably use a load balancer, but let's
OLD use this for now):
```shell
# IP for ccom.unh.edu
#$ IP_ADDRESS=132.177.103.226
#$ aws --region $AWS_REGION ec2 authorize-security-group-ingress \
#  --group-id $(cat output/create_security_group.json | jq -r '.GroupId') \
#  --ip-permissions "[{\"IpProtocol\": \"tcp\", \"FromPort\": 8000, \"ToPort\": 8000, \"IpRanges\": [{\"CidrIp\": \"${IP_ADDRESS}/32\"}]}, {\"IpProtocol\": \"tcp\", \"FromPort\": 443, \"ToPort\": 443, \"IpRanges\": [{\"CidrIp\": \"${IP_ADDRESS}/32\"}]}]" \
#  | tee output/create_security_group_rule.json
```

Create ingress rule to allow anything running in the same VPC (e.g., lambdas, EC2) to access other 
services in the subnet on port 80 (HTTPS):
```shell
$ aws --region $AWS_REGION ec2 authorize-security-group-ingress \
  --group-id $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  --ip-permissions '[{"IpProtocol": "tcp", "FromPort": 80, "ToPort": 80, "IpRanges": [{"CidrIp": "10.0.0.0/16"}]}]' \
  | tee output/create_security_group_private_rule_http.json
```

Tag the HTTP ingress rule with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/create_security_group_private_rule_http.json | jq -r '.SecurityGroupRules[0].SecurityGroupRuleId') \
  --tags 'Key=Name,Value=wibl-manager-http'
```

Create service endpoint so that things running in the VPC can access S3:
```shell
$ aws --region $AWS_REGION ec2 create-vpc-endpoint \
  --vpc-id $(cat output/create_vpc.txt) \
  --service-name "com.amazonaws.${AWS_REGION}.s3" \
  --route-table-ids $(cat output/create_route_table_ingress.txt) \
  | tee output/create_vpc_endpoint_s3.json
```

Create service endpoint so that things running in the VPC can access SNS:
```shell
$ aws --region $AWS_REGION ec2 create-vpc-endpoint \
  --vpc-id $(cat output/create_vpc.txt) \
  --vpc-endpoint-type Interface \
  --service-name "com.amazonaws.${AWS_REGION}.sns" \
  --subnet-id $(cat output/create_subnet_public.txt) \
  --security-group-ids $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  | tee output/create_vpc_endpoint_sns.json
```

Create service endpoints for ECR:
```shell
$ aws --region $AWS_REGION ec2 create-vpc-endpoint \
  --vpc-id $(cat output/create_vpc.txt) \
  --vpc-endpoint-type Interface \
  --service-name "com.amazonaws.${AWS_REGION}.ecr.dkr" \
  --subnet-id $(cat output/create_subnet_private.txt) \
  --security-group-ids $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  | tee output/create_vpc_endpoint_ecr_dkr.json
$ aws --region $AWS_REGION ec2 create-vpc-endpoint \
  --vpc-id $(cat output/create_vpc.txt) \
  --vpc-endpoint-type Interface \
  --service-name "com.amazonaws.${AWS_REGION}.ecr.api" \
  --subnet-id $(cat output/create_subnet_private.txt) \
  --security-group-ids $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  | tee output/create_vpc_endpoint_ecr_api.json
```

Create service endpoints for ECS:
```shell
$ aws --region $AWS_REGION ec2 create-vpc-endpoint \
  --vpc-id $(cat output/create_vpc.txt) \
  --vpc-endpoint-type Interface \
  --service-name "com.amazonaws.${AWS_REGION}.ecs-agent" \
  --subnet-id $(cat output/create_subnet_private.txt) \
  --security-group-ids $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  | tee output/create_vpc_endpoint_ecs_agent.json
$ aws --region $AWS_REGION ec2 create-vpc-endpoint \
  --vpc-id $(cat output/create_vpc.txt) \
  --vpc-endpoint-type Interface \
  --service-name "com.amazonaws.${AWS_REGION}.ecs-telemetry" \
  --subnet-id $(cat output/create_subnet_private.txt) \
  --security-group-ids $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  | tee output/create_vpc_endpoint_ecs_telemetry.json
$ aws --region $AWS_REGION ec2 create-vpc-endpoint \
  --vpc-id $(cat output/create_vpc.txt) \
  --vpc-endpoint-type Interface \
  --service-name "com.amazonaws.${AWS_REGION}.ecs" \
  --subnet-id $(cat output/create_subnet_private.txt) \
  --security-group-ids $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  | tee output/create_vpc_endpoint_ecs.json
```

Create service endpoint for CloudWatch:
```shell
$ aws --region $AWS_REGION ec2 create-vpc-endpoint \
  --vpc-id $(cat output/create_vpc.txt) \
  --vpc-endpoint-type Interface \
  --service-name "com.amazonaws.${AWS_REGION}.logs" \
  --subnet-id $(cat output/create_subnet_private.txt) \
  --security-group-ids $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  | tee output/create_vpc_endpoint_logs.json
```

Create service endpoint for EFS:
```shell
$ aws --region $AWS_REGION ec2 create-vpc-endpoint \
  --vpc-id $(cat output/create_vpc.txt) \
  --vpc-endpoint-type Interface \
  --service-name "com.amazonaws.${AWS_REGION}.elasticfilesystem" \
  --subnet-id $(cat output/create_subnet_private.txt) \
  --security-group-ids $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  | tee output/create_vpc_endpoint_efs.json
```

Create ingress rule to allow access to the VPC service endpoints:
```shell
$ aws --region $AWS_REGION ec2 authorize-security-group-ingress \
  --group-id $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  --ip-permissions '[{"IpProtocol": "tcp", "FromPort": 443, "ToPort": 443, "IpRanges": [{"CidrIp": "10.0.0.0/16"}]}]' \
  | tee output/create_security_group_private_rule_vpc_endpoints.json
```

Tag the VPC service endpoint ingress rule with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/create_security_group_private_rule_vpc_endpoints.json | jq -r '.SecurityGroupRules[0].SecurityGroupRuleId') \
  --tags 'Key=Name,Value=wibl-manager-vpc-svc'
```

+++++
NOTE: Experiment using this instead of the service end points

Create Elastic IP to associate with NAT gateway:
```shell
$ aws --region $AWS_REGION ec2 allocate-address | tee output/alloc_nat_gw_eip.json
````

Create NAT gateway
```shell
$ aws --region $AWS_REGION ec2 create-nat-gateway \
  --subnet-id  $(cat output/create_subnet_public.txt) \
  --connectivity-type public \
  --allocation-id "$(cat output/alloc_nat_gw_eip.json | jq -r '.AllocationId')" \
  | tee output/create_nat_gateway.json
````

TODO: Update private subnet route table to route 0.0.0.0/0 to NAT gateway
```shell
TBD
```

+++++



## Create EFS volume for use with ECS task
Create the volume:
```shell
$ aws --region $AWS_REGION efs create-file-system \
  --creation-token wibl-manager-ecs-task-efs \
  --encrypted \
  --backup \
  --performance-mode generalPurpose \
  --throughput-mode bursting \
  --tags 'Key=Name,Value=wibl-manager-ecs-task-efs' \
  | tee output/create_efs_file_system.json
```

> Note: Make sure your account has the `AmazonElasticFileSystemFullAccess` permissions policy 
> attached to it.

> Delete with 'aws efs delete-file-system --mount-target-id --file-system-id $(cat output/create_efs_file_system.json | jq -r '.FileSystemId')'

Create mount target for EFS volume within our VPC subnet:
```shell
$ aws --region $AWS_REGION efs create-mount-target \
  --file-system-id $(cat output/create_efs_file_system.json | jq -r '.FileSystemId') \
  --subnet-id $(cat output/create_subnet_private.txt) \
  --security-groups $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  | tee output/create_efs_mount_target.json
```

> Delete with `aws efs delete-mount-target --mount-target-id $(cat output/create_efs_mount_target.json | jg -r '.MountTargetId')` 

Create ingress rule to allow NFS connections from the subnet (e.g., EFS mount point):
```shell
$ aws --region $AWS_REGION ec2 authorize-security-group-ingress \
  --group-id $(cat output/create_security_group_private.json | jq -r '.GroupId') \
  --ip-permissions '[{"IpProtocol": "tcp", "FromPort": 2049, "ToPort": 2049, "IpRanges": [{"CidrIp": "10.0.0.0/24"}]}]' \
  | tee output/create_security_group_private_rule_efs.json
```

Tag the NFS ingress rule with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/create_security_group_private_rule_efs.json | jq -r '.SecurityGroupRules[0].SecurityGroupRuleId') \
  --tags 'Key=Name,Value=wibl-manager-efs-mount-point'
```

## Create task definition
Using image pushed to ECR above, create task definition:
```shell
$ AWS_EFS_FS_ID=$(cat output/create_efs_file_system.json | jq -r '.FileSystemId')
$ sed "s|REPLACEME_AWS_ACCOUNT_ID|$AWS_ACCOUNT_ID|g" input/manager-task-definition.proto | \
  sed "s|REPLACEME_AWS_EFS_FS_ID|$AWS_EFS_FS_ID|g" | \
  sed "s|REPLECEME_AWS_REGION|$AWS_REGION|g" > input/manager-task-definition.json
$ aws --region $AWS_REGION ecs register-task-definition \
	--cli-input-json file://input/manager-task-definition.json | tee output/create_task_definition.json
```

## Associate CloudWatch policy to `ecsInstanceRole` to allow ECS tasks to send logs to CloudWatch
Create policy:
```shell
$ aws --region $AWS_REGION iam create-policy \
  --policy-name 'ECS-CloudWatchLogs' \
  --policy-document file://input/ecs-cloudwatch-policy.json | tee output/create_log_policy.json
```

Attach:
```shell
$ aws --region $AWS_REGION iam attach-role-policy \
  --role-name ecsTaskExecutionRole \
  --policy-arn $(cat output/create_log_policy.json| jq -r '.Policy.Arn') \
  | tee output/attach_role_policy_cloudwatch.json
```

## Create application load balancer so that lambdas can find our WIBL manager service

Create load balancer:
```shell
$ aws elbv2 create-load-balancer --name wibl-manager-ecs-elb --type network \
  --subnets $(cat output/create_subnet_private.txt) \
  --scheme internal \
  | tee output/create_elb.json
```

> If you want to make the manager accessible over the Internet, change the `scheme` to `internet-facing`.

> Delete with `aws elbv2 delete-load-balancer --load-balancer-arn $(cat output/create_elb.json | jq -r '.LoadBalancers[0].LoadBalancerArn')`

Create target group to associate load balancer listeners to ECS Fargate elastic IP addresses:
```shell
$ aws elbv2 create-target-group --name wibl-manager-ecs-elb-tg \
  --protocol TCP --port 8000 \
  --target-type ip \
  --vpc-id $(cat output/create_vpc.txt) \
  | tee output/create_elb_target_group.json
```

> Delete with `aws elbv2 delete-target-group --target-group-arn $(cat output/create_elb_target_group.json | jq -r '.TargetGroups[0].TargetGroupArn')`

Create ELB listener:
```shell
$ aws elbv2 create-listener \
  --load-balancer-arn $(cat output/create_elb.json | jq -r '.LoadBalancers[0].LoadBalancerArn') \
  --protocol TCP \
  --port 80 \
  --default-actions \
    Type=forward,TargetGroupArn=$(cat output/create_elb_target_group.json | jq -r '.TargetGroups[0].TargetGroupArn') \
  | tee output/create_elb_listener.json
```

> Delete with `aws elbv2 delete-listener --listener-arn $(cat output/create_elb_listener.json | jq -r '.Listeners[0].ListenerArn')`

## Create ECS service
Create an ECS service in our cluster to launch one or more tasks based on our task definition:
```shell
$ SECURITY_GROUP_ID=$(cat output/create_security_group_private.json | jq -r '.GroupId')
$ SUBNETS=$(cat output/create_subnet_private.txt)
$ ELB_TARGET_GROUP_ARN=$(cat output/create_elb_target_group.json | jq -r '.TargetGroups[0].TargetGroupArn')

$ aws --region $AWS_REGION ecs create-service \
	--cluster wibl-manager-ecs \
	--service-name wibl-manager-ecs-svc \
	--task-definition wibl-manager-ecs-task \
	--desired-count 1 \
	--load-balancers "targetGroupArn=${ELB_TARGET_GROUP_ARN},containerName=wibl-manager,containerPort=8000" \
	--network-configuration "awsvpcConfiguration={subnets=[ ${SUBNETS} ],securityGroups=[ ${SECURITY_GROUP_ID} ]}" \
	--launch-type "FARGATE" | tee output/create_ecs_service.json
```

## Optional: Create EC2 instance to test access to the ELB for the ECS service from within the VPC

Create public subnet:
```shell
$ aws --region $AWS_REGION ec2 create-subnet --vpc-id $(cat output/create_vpc.txt) --cidr-block 10.0.1.0/24 \
	--query Subnet.SubnetId --output text | tee output/create_subnet_public.txt 
```

Tag the subnet with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/create_subnet_public.txt ) \
  --tags 'Key=Name,Value=wibl-public-subnet'
```

Associate the custom routing table with the public subnet:
```shell
$ aws --region $AWS_REGION ec2 associate-route-table --subnet-id $(cat output/create_subnet_public.txt) \
  --route-table-id $(cat output/create_route_table_ingress.txt)
```

Create Internet gateway:
```shell
$ aws --region $AWS_REGION ec2 create-internet-gateway --query InternetGateway.InternetGatewayId \
  --output text | tee output/create_internet_gateway.txt
```

Attach the Internet gateway to the VPC:
```shell
$ aws --region $AWS_REGION ec2 attach-internet-gateway --vpc-id $(cat output/create_vpc.txt) \
  --internet-gateway-id $(cat output/create_internet_gateway.txt)
```

> Detach with: `aws --region $AWS_REGION ec2 detach-internet-gateway --vpc-id $(cat output/create_vpc.txt) \
  --internet-gateway-id $(cat output/create_internet_gateway.txt)`
> After detaching, delete the gateway with `aws ec2 delete-internet-gateway --internet-gateway-id $(cat output/create_internet_gateway.txt)`

Create a routing table for public subnet to the VPC:
```shell
$ aws --region $AWS_REGION ec2 create-route-table --vpc-id $(cat output/create_vpc.txt) \
  --query RouteTable.RouteTableId --output text | tee output/create_route_table_ingress.txt
```

> Delete with `aws ec2 delete-route-table --route-table-id $(cat output/create_route_table_ingress.txt)` 

Create route pointing traffic to the Internet gateway:
```shell
$ aws --region $AWS_REGION ec2 create-route --route-table-id $(cat output/create_route_table_ingress.txt) \
  --destination-cidr-block 0.0.0.0/0 \
  --gateway-id $(cat output/create_internet_gateway.txt)
```

Confirm that the routing table has been setup as expected:
```shell
$ aws --region $AWS_REGION ec2 describe-route-tables --route-table-id $(cat output/create_route_table_ingress.txt)
```

Tag the route table with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/create_route_table_ingress.txt) \
  --tags 'Key=Name,Value=wibl-public-rt'
```

Create security group to give us control over ingress/egress:
```shell
$ aws --region $AWS_REGION ec2 create-security-group \
	--group-name wibl-mgr-public \
	--vpc-id $(cat output/create_vpc.txt) \
	--description "Security Group for WIBL lambdas and WIBL Manager on ECS Fargate" \
	| tee output/create_security_group_public.json
```

> Delete with `aws ec2 delete-security-group --group-id $(cat output/create_security_group_public.json | jq -r '.GroupId')`

Tag the security group with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/create_security_group_public.json | jq -r '.GroupId') \
  --tags 'Key=Name,Value=wibl-mgr-public'
```

Update security group to allow SSH ingress from trusted public IP:
```shell
# IP for ccom.unh.edu
$ IP_ADDRESS=132.177.103.226
$ aws --region $AWS_REGION ec2 authorize-security-group-ingress \
  --group-id $(cat output/create_security_group_public.json | jq -r '.GroupId') \
  --ip-permissions "[{\"IpProtocol\": \"tcp\", \"FromPort\": 22, \"ToPort\": 22, \"IpRanges\": [{\"CidrIp\": \"${IP_ADDRESS}/32\"}]}]" \
  | tee output/create_security_group_public_rule_vpc_ssh.json
```

Tag the SSH ingress rule with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/create_security_group_public_rule_vpc_ssh.json | jq -r '.SecurityGroupRules[0].SecurityGroupRuleId') \
  --tags 'Key=Name,Value=wibl-manager-ssh'
```

Launch test EC2 instance with key, security group, and subnet created above:
```shell
$ aws ec2 run-instances --image-id ami-09f3bb43b9f607008 --count 1 --instance-type t4g.nano \
	--key-name wibl-manager-key \
	--associate-public-ip-address \
	--security-group-ids \
	  $(cat output/create_security_group_public.json | jq -r '.GroupId') \
	  $(cat output/create_security_group_private.json | jq -r '.GroupId') \
	--subnet-id $(cat output/create_subnet_public.txt) \
	| tee output/run-test-ec2-instance.json
```

Tag the instance with a name:
```shell
$ aws ec2 create-tags --resources $(cat output/run-test-ec2-instance.json | jq -r '.Instances[0].InstanceId') \
  --tags 'Key=Name,Value=wibl-manager-test'
```



## References

https://docs.aws.amazon.com/AmazonECS/latest/developerguide/ECS_AWSCLI_Fargate.html
https://github.com/michaelact/example-ecs-deployment/tree/main/deploy/aws-cli
https://github.com/aws-samples/aws-containers-task-definitions/blob/master/gunicorn/gunicorn_fargate.json
https://docs.aws.amazon.com/AmazonECS/latest/developerguide/task_definitions.html
https://docs.aws.amazon.com/AmazonECS/latest/developerguide/task_definition_parameters.html
https://docs.aws.amazon.com/AmazonECS/latest/developerguide/ecs-arm64.html
https://docs.aws.amazon.com/elasticloadbalancing/latest/network/network-load-balancer-cli.html
https://docs.aws.amazon.com/AmazonECS/latest/developerguide/create-application-load-balancer.html
https://gist.github.com/reggi/dc5f2620b7b4f515e68e46255ac042a7
https://aws.amazon.com/premiumsupport/knowledge-center/ecs-pull-container-api-error-ecr/
https://docs.aws.amazon.com/AmazonECR/latest/userguide/vpc-endpoints.html#ecr-setting-up-vpc-create
https://docs.aws.amazon.com/AmazonECS/latest/developerguide/vpc-endpoints.html
