import os
import pandas as pd
from tensorflow import keras

# Base Model class
class Model:
  SEED = 2023
  def __init__(self, name, files, output, setup, overrun={}, model=None):
    keras.utils.set_random_seed(Model.SEED)

    self.model    = model
    self.setup    = setup
    self.output   = output
    self.log_dir  = self.output+"/log"
    self.files    = files
    self.name     = name
    self.overrun  = overrun

    self.CFG      = __import__(self.setup.replace('/', '.').strip('.py'), fromlist=[''])

    self.SETUP    = self.CFG.SETUP
    self.FEATURES = self.CFG.FEATURES

    self.target     = self.SETUP['target'     ] if 'target'     in self.SETUP.keys() else 'target'
    self.max_events = self.SETUP['max_events' ] if 'max_events' in self.SETUP.keys() else None

    assert self.target not in self.FEATURES , "The target variable is in the feature list"

    if not os.path.exists:
      os.makedirs(self.output)

  # Prediction method
  def predict(self, batch_size=500):
    for file in self.files:
      print("Running prediction on", file)
      path = self.output+"/"+os.path.basename(file)
      data = pd.read_hdf(file)[self.FEATURES]
      pred = self.model.predict(data, batch_size=batch_size)
      pd.DataFrame({'predictions': pred.reshape(len(pred))}).to_hdf(path, key='prediction')

  # Load input file
  def load(self):
    self.inputs = {
      os.path.basename(h).strip('.h5'): pd.read_hdf(h).sample(frac=1, random_state=Model.SEED)[:self.max_events] for h in self.files
    } if self.max_events is not None else {
      os.path.basename(h).strip('.h5'): pd.read_hdf(h).sample(frac=1, random_state=Model.SEED) for h in self.files
    }
    for k, v in self.inputs.items():
      v['sample'] = k
    self.dframe = pd.concat(self.inputs.values()).reset_index()
    self.x_train = self.dframe.loc[self.dframe['is_train']==1, self.FEATURES]
    self.x_valid = self.dframe.loc[self.dframe['is_valid']==1, self.FEATURES]
    self.x_test  = self.dframe.loc[self.dframe['is_test' ]==1, self.FEATURES]
    self.y_train = self.dframe.loc[self.dframe['is_train']==1, self.target  ]
    self.y_valid = self.dframe.loc[self.dframe['is_valid']==1, self.target  ]
    self.y_test  = self.dframe.loc[self.dframe['is_test' ]==1, self.target  ]

  # Fitting method
  def fit(self, callbacks=[]):
    self.FIT_SETUP = self.SETUP['FIT']
    self.model.fit(
      self.x_train, self.y_train                    ,
      validation_data = [self.x_valid, self.y_valid],
      callbacks       = callbacks                   ,
      **self.FIT_SETUP
    )
    self.model.save(self.output)

  @staticmethod
  def OVERRUN(ori, rep):
    for k, v in rep.items():
      if type(v)==dict: OVERRUN(ori[k], rep[k])
      ori[k] = rep[k]