import os
basedir = os.path.abspath(os.path.dirname(__file__))


class Config():
    #This is where the configurations for the applications are stored.

    SECRET_KEY = os.environ.get('SECRET_KEY') or 'you-will-never-guess'
    #Flask and some of its extensions use the value of the secret key
    # as a cryptographic key, useful to generate signatures or tokens.

    #configuration as related to database implementations
    SQLALCHEMY_DATABASE_URI = os.environ.get('DATABASE_URL') or \
        'sqlite:///' + os.path.join(basedir, 'fairBilling.db')
    SQLALCHEMY_TRACK_MODIFICATIONS = False
