import setuptools

long_description = 'Python code for Trusted Community and Crowd Sourced Bathymetry'

setuptools.setup(
    name="wibl-python",
    version="0.0.1",
    author="Brian Calder",
    author_email="brc@ccom.unh.edu",
    description="Python code WIBL low-cost data logger system",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.8',
    install_requires=[
        'boto3>=1.20',
        'requests>=2',
        'numpy>=1.20',
        'pynmea2>=1.18'
    ],
    tests_require=[
    ],
    entry_points={
        'console_scripts': [
    ]},
    include_package_data=False,
    zip_safe=False
)
