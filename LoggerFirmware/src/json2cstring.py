import json
with open('boot_config.json') as f:
    config = json.load(f)
cstyle_string = json.dumps(config).replace('"', '\\"')
print(f'static const char *stable_config = \"{cstyle_string}\"')
