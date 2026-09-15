DDM — Director SIAG

Contenido de este README:
 -> Organización del repositorio y flujo de trabajo (git) — abajo.
 -> Instalación en una máquina nueva — al final del archivo.
 -> Configuración de despliegue (ddm.ini / opendds.ini), cómo correr DDM y
    diagnóstico de fallos — al final del archivo.

Si venís a compilar el proyecto por primera vez, andá directo a "Instalación en
una máquina nueva": clonar el repo no alcanza.

---

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

---

*Instalación en una máquina nueva*

Clonar el repo no alcanza: DDM linkea contra ReplicationEngine (RE), que viene
como submódulo y necesita OpenDDS compilado en la máquina.

1. Acceso a los DOS proyectos de GitLab

El submódulo vive en un proyecto distinto del de DDM y se clona por SSH:

    DDM -> myametti/ddm-directorsiag
    RE  -> rnavarro/replicationengine

La máquina necesita su clave SSH cargada en gitlabsiag y permiso de lectura en
los dos. Si falta el permiso sobre RE, el clone parece andar pero
third_party/replicationengine queda vacío y el build falla por headers que no
aparecen.

2. Clonar con el submódulo

    git clone --recurse-submodules git@gitlabsiag.armada.mil.ar:myametti/ddm-directorsiag.git
    cd ddm-directorsiag

Si ya habían clonado sin --recurse-submodules:

    git submodule update --init --recursive

3. Dependencias

    -> g++ con soporte C++17
    -> cmake >= 3.16 (lo usa RE; DDM.pro lo invoca solo)
    -> libsqlite3-dev
    -> Qt 6.7.3 (en las máquinas actuales está en /opt/qt/6.7.3/gcc_64;
       ese qmake tiene que quedar en el PATH)

4. OpenDDS 3.33.0

Es el paso largo: no hay paquete, se compila de fuente y tarda. Se configuró
sin ningún flag especial:

    tar xzf OpenDDS-3.33.0.tar.gz
    cd OpenDDS-3.33.0
    ./configure
    make -j$(nproc)

5. Variables de entorno (en el ~/.bashrc de esa máquina)

    source ~/OpenDDS-3.33.0/setenv.sh
    export OPENDDS_CONFIG_DIR=<ruta-del-checkout>/third_party/replicationengine/config

El setenv.sh exporta DDS_ROOT, ACE_ROOT y LD_LIBRARY_PATH. DDS_ROOT es el que
manda: DDM.pro lo mira para decidir si compila con DDS real o en modo stub.
OPENDDS_CONFIG_DIR se explica en la sección de configuración.

6. Compilar

    qmake DDM.pro
    make -j$(nproc)

DDM.pro configura y compila RE por CMake solo; no hay que buildear el submódulo
a mano.

OJO: si DDS_ROOT no está definido, el build NO falla. Compila RE en modo stub y
queda una consola que arranca bien pero no replica nada. El aviso no está en el
build sino en el log de arranque.


*Configuración de despliegue*

Hay dos archivos .ini, ninguno versionado (cada consola tiene el suyo). En el
repo están las plantillas: ddm.ini.example y opendds.ini.example.

Se buscan en lugares DISTINTOS, y esto es la fuente número uno de confusión:

    ddm.ini     -> junto al ejecutable (QCoreApplication::applicationDirPath)
    opendds.ini -> en el directorio de trabajo del proceso, o en el que indique
                   la variable OPENDDS_CONFIG_DIR

O sea: ddm.ini sigue al binario, opendds.ini sigue al cwd. Dejar opendds.ini al
lado del ejecutable NO alcanza si DDM se lanza desde otro directorio. Por eso se
recomienda definir OPENDDS_CONFIG_DIR y olvidarse.

    cp ddm.ini.example build/ddm.ini      # y completar los valores
    cp opendds.ini.example <dir>/opendds.ini   # o usar OPENDDS_CONFIG_DIR

Valores de ddm.ini:

    -> domain_id: es EL parámetro crítico. Tiene que ser el mismo en todas las
       consolas que se deban ver entre sí; distinto domain_id son redes DDS
       separadas que no se descubren, y no hay ningún error que lo indique.
       Es obligatorio: si falta o no es un entero, DDM aborta el arranque a
       propósito (RF-DDS-006), para no mezclar una consola de simulación con la
       red de combate real.
    -> console_id: identifica esta consola y viaja como source_console_id en
       cada objeto replicado. Es un campo de AUDITORÍA: no participa de la
       resolución de conflictos (el ConflictResolver es last-write-wins por
       last_updated) y los GUID son UUID v4, independientes de la consola.
       Aunque no participe del LWW, TIENE QUE SER ÚNICO EN LA RED: dos consolas
       con el mismo console_id no se ven entre sí — cada una descarta lo que
       publica la otra, y no hay error ni línea de log que lo avise. Si falta,
       se usa 0 con un warning, así que dos consolas que se lo olvidan quedan
       las dos en 0 y tampoco se ven. Convención de asignación: el último octeto
       de la IP de la consola (10.0.0.231 -> console_id=231).

opendds.ini fija descubrimiento RTPS puro y transporte rtps_udp (SAD 4.3 de RE).
Los valores de la plantilla son los de referencia de RE
(third_party/replicationengine/config/opendds.ini); mantenerlos alineados.

Red: el descubrimiento es RTPS puro (SPDP/SEDP) sobre multicast, sin
DCPSInfoRepo. Los switches entre consolas tienen que enrutar multicast (IGMP
snooping). Dos consolas bien configuradas pero en segmentos sin multicast no se
ven entre sí.


*Correr DDM y verificar que replica*

    cd build && ./DDM

Un arranque sano se ve así:

    Consola lista (help | exit)
    [DDSTransport] conectado al dominio 99, tópico RealTimeReplication

Si esa segunda línea no aparece, la consola NO está replicando aunque el prompt
funcione. Conviene mirarla siempre.


*Diagnóstico de los fallos más comunes*

1. "no se pudo leer el archivo de configuración opendds.ini"

Falta opendds.ini en el cwd y OPENDDS_CONFIG_DIR no está definida. Es el caso
típico de lanzar el binario desde otro directorio.

IMPORTANTE: esto NO aborta DDM. A diferencia de un domain_id ausente, acá la
consola levanta, sirve el estado local que tenga en SQLite y queda en
NetworkStatus::TRANSPORT_FAILED: sin replicar, pero con aspecto de normal. El
único indicio es esa línea de error al arrancar.

2. La consola arranca pero nunca ve a las demás

Revisar, en este orden: (a) que el domain_id sea el mismo en todas; (b) que
apareció la línea "conectado al dominio N"; (c) que la red enrute multicast;
(d) que no haya dos consolas con el mismo console_id.

El síntoma distingue los casos: si la consola no ve a NINGUNA, es domain_id o
multicast. Si ve a todas MENOS A UNA, es console_id repetido con esa. Un
console_id duplicado sólo te deja ciego respecto de tu gemela; los otros dos
fallos te aíslan de todos.

3. "Configuration: no se encontró .../ddm.ini" y DDM sale con código 1

Falta ddm.ini junto al ejecutable. Es intencional que muera acá (RF-DDS-006).

4. Compila y arranca, pero DDSTransport nunca dice nada

Se compiló en modo stub porque DDS_ROOT no estaba definido al correr qmake.
Aplicar el setenv.sh, y después qmake y make de nuevo.


*Estado local*

DDM crea tactical_db.db (ObjectStorage de RE) en el directorio de trabajo al
arrancar, así que aparece donde se haya lanzado el binario. Está en .gitignore
junto con sus archivos -journal/-wal/-shm.
