# Localize os headers
EMLEARN_INCLUDE="/home/italolanza/.cache/pypoetry/virtualenvs/projeto-tg-tKWSB8r1-py3.12/lib/python3.12/site-packages/emlearn/"

# Compile
gcc test_model.c -o classificador -I$EMLEARN_INCLUDE -I.. -lm