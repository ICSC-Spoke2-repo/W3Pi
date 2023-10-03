from tensorflow    import keras
from keras         import Sequential
from keras.layers  import InputLayer, Dense, Dropout
from config.Model  import Model

# Fully Connected model inheriting from Model
class FCModel(Model):

    # Lambdas to normalize input data
    MEAN = lambda x: x.mean() if x.dtype!='int32' else 0
    VAR  = lambda x: x.var()  if x.dtype!='int32' else 1

    # Init FCModel
    def __init__(self, dropout, neurons, n_targets=1, **kwargs):
        super().__init__(**kwargs)
        self.dropout = dropout
        self.neurons = neurons
        self.n_targets = n_targets

    # Compile method
    def compile(self):
        self.DENSE_SETUP   = self.SETUP['DENSE'  ]
        self.LAST_SETUP    = self.SETUP['LAST'   ]
        self.COMPILE_SETUP = self.SETUP['COMPILE']

        # Define a sequential model
        self.model = Sequential(
            InputLayer(input_shape=len(self.FEATURES)),
            name=self.name
        )

        # Add first, non-trainable, layer to normalize input data
        self.model.add(
            keras.layers.Normalization(
                axis = -1,
                mean      = [FCModel.MEAN(self.dframe[k]) for k in self.FEATURES],
                variance  = [FCModel.VAR (self.dframe[k]) for k in self.FEATURES]
            )
        )
        self.model.layers[-1].trainable = False

        # Add hidden layers
        for n in self.neurons:
            self.model.add(Dense(n, **self.DENSE_SETUP))
            if self.dropout is not None:
                self.model.add(Dropout(self.dropout))

        # Add output layer
        self.model.add(Dense(self.n_targets, **self.LAST_SETUP))

        # Print summary of the NN
        print(self.model.summary())

        # Compile the module
        self.model.compile(**self.COMPILE_SETUP)
