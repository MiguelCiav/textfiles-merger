# Información del Proyecto 2

## Datos Personales
- **Nombres:**  [Henry Manuel] [Miguel Angel] [Nohelyn]
- **Apellidos:** [Arteaga Goncalves] [Ciavato Monsalve] [Mejias]
- **Cédula de Identidad:** [26921110] [30541929] [20173834]
---

## Enlace a la documentación del proyecto

https://docs.google.com/document/d/1PymUvEZmcS09ObWLUJfP5L9LJWzMIC63rx7P4iTC-OI/edit?usp=sharing

## Errores conocidos y/o funcionalidades faltantes, de haberlas.

- Para caracteres especiales se desempata utilizando el código ASCII debido al criterio de la función strcasecmp.
- En el codigo, las comparaciones para verificar duplicados hacen uso de strcmp, sensible a mayúsculas, por lo que, strings como 'casa' y 'cAsa', no se consideran duplicados.