# -*- coding: utf-8 -*-
"""ModeloPrediccion.ipynb

Automatically generated by Colaboratory.

Original file is located at
    https://colab.research.google.com/drive/1PF6h7SFFQPccPJ7Lxl-J-xTMZGRo1Rt7

# Predicción Data SAJR-1221
##### Integrantes: ***Jaime Lemus***
"""

import numpy as np
import matplotlib.pyplot as plt 
import pandas as pd

#Importamos la data
df = pd.read_csv('DataProcesada.csv')

#Prediccion
from sklearn.tree import DecisionTreeRegressor
from sklearn.svm import SVR
from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score
from sklearn.linear_model import LinearRegression

X = df['CO_ppm'].values.reshape(-1,1)
y = df['NOX_ppb'].values.reshape(-1,1)
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.3, random_state=0)
regressor = LinearRegression()  
regressor.fit(X_train, y_train)
y_pred = regressor.predict(X_test)
RLprecision = regressor.score(X_train, y_train)
RLprecision = round(RLprecision,2)
print("La regresión lineal =", RLprecision*100)

X = np.array(df[["CO_ppm", "NO2_ppb"]]) 
x = X[:,0]
y = X[:,1]
x = x.reshape(8170,1)
y = y.reshape(8170,1)
X_train, X_test, y_train, y_test = train_test_split(x, y, test_size = 0.2, random_state = 44)
model = DecisionTreeRegressor()
model.fit(X_train, y_train)
predictions = model.predict(X_test)
errors = abs(predictions - y_test)
aux = 100*(errors / y_test)
RLTree = np.mean(aux)
RLTree = round(RLTree, 2)
print("Precision del modelo: ", RLTree)

X = np.array(df[["CO_ppm", "NO2_ppb"]])
x = X[:,0]
y = X[:,1]
x = x.reshape(8170,1)
y = y.reshape(8170,1)
X_train, X_test, y_train, y_test = train_test_split(x, y, test_size = 0.2, random_state = 44)
regressor = SVR(kernel = "rbf")
regressor.fit(X_train,y_train)
y_predSVR = regressor.predict(X_test)
#calcular la precision del modelo
errors = abs(y_predSVR - y_test)
aux = 100*(errors / y_test)
SVRpresicion = np.mean(aux)
SVRpresicion = round(SVRpresicion, 2)
print("Precision del modelo: ", SVRpresicion)