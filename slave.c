
// El esclavo va recibiendo PATHs de archivos por la entrada estandar

// Para cada archivo crea un fork() y un execve() para ejecutar hashmd5 de ese archivo
// notar que hashmd5 imprime el resultado junto con el nombre del archivo en la salida estandar
// Si queremos que se imprima el PID del esclavo lo podemos imprimir justo antes de ejecutar hashmd5

// Si el master cierra el pipe de escritura, el esclavo lee EOF y debe terminar
