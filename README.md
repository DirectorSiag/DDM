*Organización del Repositorio y Flujo de Trabajo (CI/CD)*

Para asegurar la calidad del proyecto DDM y evitar errores en la administración del código, vamos a usar la estrategia Feature Branch Workflow y Gitflow. El objetivo de esta forma de organización y estrategia es mantener un desarrollo limpio y organizado.
 1. Estructura de Ramas Principales
    El repositorio se organiza con las siguientes ramas:
        -> main / master: Es el código de producción. Solo contiene código estable, testeado y funcional. NO se programa directamente aca ni se sube directamente el codigo aca.
        -> develop: Es la rama principal de integración y desarrollo. Acá se unen todas las nuevas funcionalidades antes de pasar a producción. Todo el trabajo nuevo parte de acá.

2. Ramas Temporales (De Trabajo)
    Cuando necesiten crear algo nuevo siempre tienen que crear una rama a partir de develop usando los siguientes prefijos:
    -> feature/nombre-funcionalidad: Para nuevas funcionalidades (ej. feature/sitrep, feature/figuras).
    -> hotfix/nombre-hotfix: Para corregir errores críticos y urgentes (ej. hotfix/bug-sitrep).
    -> release/: Para preparar una nueva versión antes de mandarla a main.

3. Flujo de Trabajo Obligatorio (El Ciclo de Vida del Código)
    Nadie puede subir código directamente a main ni a develop. El flujo de trabajo que tienen que seguir es:
    -> Actualizar entorno: git pull origin develop
    -> Crear la rama: git checkout -b feature/nombre-tarea
    -> Hacer commits.
    -> Subir rama: git push -u origin feature/nombre-tarea 
    -> Crear un Pull Request (PR): En GitHub, abri un PR apuntando la nueva rama hacia develop

4. Merge con la rama main
    Una vez que creen que hicieron las suficientes funcionalidades para cumplir con una nueva version del proyecto tienen que:
    -> Crear una rama release/... a partir de develop, aca solo corrigen detalles.
    -> Abren un PR desde la nueva rama release hacia la main, NO HACEN PUSH DIRECTO
    -> Esperan a la revision
    -> Corre el pipeline de CICD, tiene que correr todas las pruebas y dar verde
    -> Una vez que se aprueba el PR y se mergea a main, agregan una tag con el numero de la version

*Como seguir luego de haber renombrado una rama*
Renombré las ramas que tenian en GitHub (fijense ahi como estan por las dudas):

dev -> develop (siempre trabajan desde esta)
devLink -> feature/link
devFiguras -> feature/figuras
cpa -> feature/cpa
refactorARQ -> feature/refactor-arq
SITREP -> feature/sitrep

Para que no les de error al subir cambios tienen que actualizar (no van a perder cambios).
1. Guarden lo que estaban haciendo con un commit
    -> git add .
    -> git commit -m ...
2. Para limpiar las ramas viejas: git fetch --prune
3. Si van a empezar algo nuevo o querían ir a la rama principal: git checkout develop
4. SI ESTABAN TRABAJANDO EN UNA RAMA VIEJA (y tienen cambios sin subir): Tienen que renombrar su rama local para que se vuelva a conectar con la nueva de GitHub. Por ejemplo, si estaban en cpa, ejecuten esto (reemplacen con el nombre de su rama):
-> git checkout cpa (se paran en su rama)
-> git branch -m feature/cpa (le actualizan el nombre local)
-> git branch --set-upstream-to=origin/feature/cpa (la reconectan con el servidor, con esto ya esta actualizado)
-> (Opcional) git push origin feature/cpa (por si quieren subir los cambios, sino sigan con lo suyo y cuando quieran suben con git add y commit)
